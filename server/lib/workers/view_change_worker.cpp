#include "lib/workers/view_change_worker.h"

void ViewChangeWorker::process_request(ViewChangeRequest req) {
    int next_view_number = req.next_view_number();
    int current_view_number = server_state.get_current_view_number();

    // std::cout<<"IN VIEW CHANGE WORKER"<<std::endl;
    log_store.add_view_change_request(req);
    if(log_store.new_view_request_exists(next_view_number) || log_store.is_view_change_majority_reached(next_view_number)) {
        // std::cout<<"HERE!"<<std::endl;
        return;
    } else {
        if(log_store.view_change_count(next_view_number) >= view_change_majority_count) {
            // std::cout<<"VIEW CHANGE MAJORITY REACHED: "<<next_view_number<<std::endl;
            server_state.update_state({{"view_change", "true"}});
            log_store.view_change_majority_reached(next_view_number);
            
            if (next_view_number % SERVER_HOSTS.size() == server_id) {
                if(!is_server_faulty) {
                    NewViewRequest new_view_request;
                    new_view_request.set_server_id(server_id);
                    new_view_request.set_view_number(next_view_number);

                    std::unordered_map<int, ViewChangeRequest> view_change_requests = log_store.get_view_change_requests(next_view_number);
                    int min_s = 0;

                    for (const auto& [_, view_change_request] : view_change_requests) {
                        *new_view_request.add_view_change_requests() = view_change_request;
                        min_s = std::max(min_s, view_change_request.checkpoint_sequence_number());
                    }

                    std::unordered_map<int, PrePrepareRequest> next_view_preprepares;
                    int max_s = 0;

                    for (const auto& [_, view_change_request] : view_change_requests) {
                        const auto& prepared_requests = view_change_request.prepare_requests();

                        for (const auto& prepared_req : prepared_requests) {
                            const PrePrepareRequest& preprepare_request = prepared_req.preprepare_request();
                            next_view_preprepares.try_emplace(preprepare_request.sequence_number(), preprepare_request);
                            // std::cout<<"PREPREPARE_REQUEST: "<<preprepare_request.view_number()<<", "<<preprepare_request.sequence_number()<<", "<<preprepare_request.message().sender()<<", "<<preprepare_request.message().receiver()<<", "<<preprepare_request.message().amount()<<std::endl;
                            max_s = std::max(max_s, preprepare_request.sequence_number());
                        }
                    }
                    // std::cout<<"HERE:: "<<min_s<<", "<<max_s<<std::endl;
                    for(int seq = min_s + 1; seq <= max_s; seq++) {
                        if(next_view_preprepares.find(seq) != next_view_preprepares.end()) {
                            PrePrepareRequest updated_preprepare_request = next_view_preprepares[seq];

                            updated_preprepare_request.set_view_number(next_view_number);
                            updated_preprepare_request.set_server_id(server_id);
                            set_signature(updated_preprepare_request);

                            *new_view_request.add_preprepare_requests() = updated_preprepare_request;
                        } else {
                            // std::cout<<"ADDING EMPTY REQUEST"<<std::endl;
                            PrePrepareRequest empty_request;
                            empty_request.set_view_number(next_view_number);
                            empty_request.set_sequence_number(seq);
                            empty_request.set_is_nop(true);
                            *new_view_request.add_preprepare_requests() = empty_request;
                        }
                    }

                    new_view_worker.push_request(new_view_request);
                    for (const auto& server_stub : SERVER_STUBS) {
                        if (server_stub) {
                            grpc::ClientContext context;
                            google::protobuf::Empty response;
                            grpc::Status status = server_stub->NewView(&context, new_view_request, &response);

                            if (!status.ok()) {
                                std::cerr << "New View RPC failed: " << status.error_message() << std::endl;
                            }
                        }
                    }
                }
            } else {
                std::thread([this, next_view_number]() {
                    const int check_interval_ms = 10;
                    std::chrono::milliseconds timeout_ms(150);
                    auto start_time = std::chrono::steady_clock::now();

                    while (this->server_state.get_current_view_number() < next_view_number && (std::chrono::steady_clock::now() - start_time) <= timeout_ms) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                    }

                    if(this->server_state.get_current_view_number() < next_view_number) {
                        // std::cout<<"SENDING NEXT VIEW CHANGE REQUEST: "<<next_view_number+1<<std::endl;
                        ViewChangeRequest retry_view_change_request;
                        retry_view_change_request.set_next_view_number(next_view_number + 1);

                        auto last_checkpoint = this->log_store.get_last_stable_checkpoint();
                        retry_view_change_request.set_checkpoint_sequence_number(last_checkpoint.second);

                        auto checkpoint_map = this->log_store.get_checkpoint_requests(last_checkpoint);
                        for (const auto& [server_id, checkpoint] : checkpoint_map) {
                            CheckpointRequest* new_checkpoint = retry_view_change_request.add_checkpoint_requests();
                            *new_checkpoint = checkpoint;
                        }

                        for (int seq_num = last_checkpoint.second + 1; seq_num <= server_state.get_last_received_sequence() ; seq_num++) {
                            if (this->log_store.preprepare_exists(server_state.get_current_view_number(), seq_num)) {
                                ViewChangePrepareRequests* prepare_group = retry_view_change_request.add_prepare_requests();
                                *prepare_group->mutable_preprepare_request() = this->log_store.preprepare_request(server_state.get_current_view_number(), seq_num);
                                auto prepares = this->log_store.prepare_requests(server_state.get_current_view_number(), seq_num);
                                for (const auto& [server_id, prepare_req] : prepares) {
                                    PrepareRequest* new_prepare = prepare_group->add_prepare_requests();
                                    *new_prepare = prepare_req;
                                }
                            }
                        }
                        
                        retry_view_change_request.set_server_id(server_id);
                        // set_signature(retry_view_change_request);
                        
                        this->push_request(retry_view_change_request);

                        for (const auto& server_stub : SERVER_STUBS) {
                            if (server_stub) {
                                grpc::ClientContext context;
                                google::protobuf::Empty response;
                                grpc::Status status = server_stub->ViewChange(&context, retry_view_change_request, &response);

                                if (!status.ok()) {
                                    std::cerr << "Retry View Change RPC failed: " << status.error_message() << std::endl;
                                }
                            }
                        }
                    }
                }).detach();
            }
        }
    }
}