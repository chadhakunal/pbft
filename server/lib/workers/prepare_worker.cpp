#include "lib/workers/prepare_worker.h"

void PrepareWorker::process_request(PrepareRequest req) {
    if(!is_server_faulty) {
        int received_view_number = req.view_number();
        int received_sequence_number = req.sequence_number();
        std::string message_digest = req.message_digest();
        
        if(received_view_number == server_state.get_current_view_number() && log_store.is_digest_valid(received_view_number, received_sequence_number, message_digest)) {
            if(!server_state.is_leader() && !verify_signature(req)) {
                std::cout<<"Invalid Signature"<<std::endl;
                return;
            }

            log_store.add_prepare_log(req);
            if(log_store.get_preprepare_request_state(received_view_number, received_sequence_number) == PrePrepareRequestState::Init) {
                if(log_store.prepare_count(received_view_number, received_sequence_number) >= prepare_majority_count) {
                    log_store.set_preprepare_request_state(received_view_number, received_sequence_number, PrePrepareRequestState::Prepared);
                    
                    if(!is_server_faulty) {
                        std::thread([received_view_number, received_sequence_number]() {
                            int timeout_counter = 5;
                            LogStore& log_store = LogStore::get_instance();
                            ServerStateData& server_state = ServerStateData::get_instance();
                            PreparedWorker& prepared_worker = PreparedWorker::get_instance();
                            ExecuteWorker& execute_worker = ExecuteWorker::get_instance();

                            while (timeout_counter > 0) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Sleep for 10 ms
                                timeout_counter -= 2;
                            }

                            if(log_store.prepare_count(received_view_number, received_sequence_number) == total_majority_count) {
                                log_store.set_preprepare_request_state(received_view_number, received_sequence_number, PrePrepareRequestState::Committed);

                                CommittedRequest request;
                                *request.mutable_preprepare_request() = log_store.preprepare_request(received_view_number, received_sequence_number);

                                for (const auto& prepare_request : log_store.prepare_requests(received_view_number, received_sequence_number)) {
                                    *request.add_prepare_requests() = prepare_request.second;
                                }
                                
                                if(server_state.is_leader()) {
                                    execute_worker.push_request(request);
                                }
                                
                                for (const auto& server_stub : SERVER_STUBS) {
                                    if (server_stub) {
                                        grpc::ClientContext context;
                                        google::protobuf::Empty response;
                                        grpc::Status status = server_stub->Committed(&context, request, &response);

                                        if (!status.ok()) {
                                            std::cerr << "Committed RPC failed: " << status.error_message() << std::endl;
                                        }
                                    }
                                }
                            } else {
                                PreparedRequest request;
                                *request.mutable_preprepare_request() = log_store.preprepare_request(received_view_number, received_sequence_number);
                                
                                for (const auto& prepare_request : log_store.prepare_requests(received_view_number, received_sequence_number)) {
                                    *request.add_prepare_requests() = prepare_request.second;
                                }

                                if(server_state.is_leader()) {            
                                    prepared_worker.push_request(request);
                                }

                                for (const auto& server_stub : SERVER_STUBS) {
                                    if (server_stub) {
                                        grpc::ClientContext context;
                                        google::protobuf::Empty response;
                                        grpc::Status status = server_stub->Prepared(&context, request, &response);

                                        if (!status.ok()) {
                                            std::cerr << "Prepared RPC failed: " << status.error_message() << std::endl;
                                        }
                                    }
                                }
                            }
                        }).detach();
                    }
                }
            }
        }
    }
}