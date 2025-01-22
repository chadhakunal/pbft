#include "lib/pbft_client_service_impl.h"

Status PbftClientServiceImpl::SendTransaction(ServerContext* context, const TransactionRequest* req, TransactionResponse* res) {
    if(is_server_running) {
        std::int64_t timestamp = req->transaction().timestamp();
        int client_id = req->transaction().client_id();
        if(log_store.transaction_reply_exists(client_id, timestamp)) {
            std::pair<TransactionReplyRequest, TransactionReplyRequestData> transaction_reply = log_store.get_transaction_reply(client_id, timestamp);
            if(transaction_reply.second.processed) {
                google::protobuf::Empty response;
                grpc::ClientContext context;

                grpc::Status status = CLIENT_STUBS[client_id]->TransactionReply(&context, transaction_reply.first, &response);
                return Status::OK;
            }
        }

        if(server_state.in_view_change()) {
            return Status::OK;
        }

        if(server_state.is_leader()) {
            if(!log_store.transaction_reply_exists(client_id, timestamp)) {
                request_worker.push_request(*req);
                log_store.add_transaction_reply(client_id, timestamp);
            }
        } else {
            int current_leader_id = server_state.get_current_view_number() % SERVER_HOSTS.size();
            grpc::ClientContext context;
            grpc::Status status = SERVER_STUBS[current_leader_id]->ForwardTransaction(&context, *req, res);
            int initial_current_view_number = server_state.get_current_view_number();

            std::thread([client_id, timestamp, initial_current_view_number]() {
                int timeout_counter = 1000;
                LogStore& log_store = LogStore::get_instance();
                ServerStateData& server_state = ServerStateData::get_instance();
                ViewChangeWorker& view_change_worker = ViewChangeWorker::get_instance();

                while (timeout_counter > 0) {
                    if (log_store.transaction_reply_exists(client_id, timestamp)) {
                        std::pair<TransactionReplyRequest, TransactionReplyRequestData> transaction_reply = log_store.get_transaction_reply(client_id, timestamp);
                        if(transaction_reply.second.processed) {
                            // std::cout<<"TRANSACTION PROCESSED"<<std::endl;
                            return;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Sleep for 10 ms
                    timeout_counter -= 10;
                }

                int next_view_number = initial_current_view_number + 1;
                if(!log_store.my_view_change_exists(next_view_number)) {
                    ViewChangeRequest view_change_request;
                    view_change_request.set_next_view_number(next_view_number);

                    auto last_checkpoint = log_store.get_last_stable_checkpoint();
                    view_change_request.set_checkpoint_sequence_number(last_checkpoint.second);

                    auto checkpoint_map = log_store.get_checkpoint_requests(last_checkpoint);
                    for (const auto& [server_id, checkpoint] : checkpoint_map) {
                        CheckpointRequest* new_checkpoint = view_change_request.add_checkpoint_requests();
                        *new_checkpoint = checkpoint;
                    }

                    for (int seq_num = last_checkpoint.second + 1; seq_num <= server_state.get_last_received_sequence(); seq_num++) {
                        if (log_store.preprepare_exists(server_state.get_current_view_number(), seq_num)) {
                            ViewChangePrepareRequests* prepare_group = view_change_request.add_prepare_requests();
                            *prepare_group->mutable_preprepare_request() = log_store.preprepare_request(server_state.get_current_view_number(), seq_num);

                            auto prepares = log_store.prepare_requests(server_state.get_current_view_number(), seq_num);
                            for (const auto& [server_id, prepare_req] : prepares) {
                                PrepareRequest* new_prepare = prepare_group->add_prepare_requests();
                                *new_prepare = prepare_req;
                            }
                        }
                    }

                    view_change_request.set_server_id(server_id);
                    // set_signature(view_change_request);

                    view_change_worker.push_request(view_change_request);
                    for (const auto& server_stub : SERVER_STUBS) {
                        if (server_stub) {
                            grpc::ClientContext context;
                            google::protobuf::Empty response;
                            grpc::Status status = server_stub->ViewChange(&context, view_change_request, &response);

                            if (!status.ok()) {
                                std::cerr << "View Change RPC failed: " << status.error_message() << std::endl;
                            }
                        }
                    }
                }
            }).detach();
        }
    }
    return Status::OK;
}

Status PbftClientServiceImpl::ServerDown(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) {
    std::cout<<"Marking Server Down!"<<std::endl;
    is_server_running = false;
    return Status::OK;
}

Status PbftClientServiceImpl::ServerUp(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) {
    std::cout<<"Marking Server Up!"<<std::endl;
    is_server_running = true;
    return Status::OK;
}

Status PbftClientServiceImpl::MakeFaulty(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) {
    std::cout<<"Marking Server Faulty!"<<std::endl;
    is_server_faulty = true;
    return Status::OK;
}

Status PbftClientServiceImpl::MakeNonFaulty(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) {
    std::cout<<"Marking Server Non Faulty!"<<std::endl;
    is_server_faulty = false;
    return Status::OK;
}

Status PbftClientServiceImpl::ResetServer(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) {
    std::cout<<"Resetting Server"<<std::endl;
    client_data.reset_balances();
    server_state.reset_server_states();
    log_store.reset_log_store();
    {
        std::lock_guard<std::mutex> lock(performance_mutex);
        processed_transactions = 0;
        total_latency = 0;
    }
    return Status::OK;
}

Status PbftClientServiceImpl::PrintLog(ServerContext* context, const google::protobuf::Empty* req, PrintLogResponse* res) {
    *res = log_store.get_print_log_response();
    return Status::OK;
}

Status PbftClientServiceImpl::PrintDB(ServerContext* context, const google::protobuf::Empty* req, PrintDBResponse* res) {
    for (const auto& entry : client_data.get_all_balances()) {
        BalanceEntry* balance_entry = res->add_balance();
        balance_entry->set_client_id(entry.first);
        balance_entry->set_amount(entry.second);
    }

    return Status::OK;
}

Status PbftClientServiceImpl::PrintStatus(ServerContext* context, const PrintStatusRequest* req, PrintStatusResponse* res) {
    int sequence_number = req->sequence_number();
    int view_number = server_state.get_current_view_number();

    if(sequence_number <= log_store.get_last_stable_checkpoint().second) {
        res->set_status(PrePrepareRequestState::Executed);
    } else if(log_store.preprepare_exists(view_number, sequence_number)) {
        res->set_status(log_store.get_preprepare_request_state(view_number, sequence_number));
    } else {
        res->set_status(-1);
    }

    return Status::OK;
}

Status PbftClientServiceImpl::PrintView(ServerContext* context, const google::protobuf::Empty* req, PrintViewResponse* res) {
    for (const auto& [key, new_view_request] : log_store.get_new_view_requests()) {
        *res->add_new_view_request() = new_view_request;
    }
    return Status::OK;
}

Status PbftClientServiceImpl::Performance(ServerContext* context, const google::protobuf::Empty* req, PerformanceResponse* res) {
    std::lock_guard<std::mutex> lock(performance_mutex);
    res->set_throughput(static_cast<double>(processed_transactions * 1000) / static_cast<double>(total_latency));
    res->set_latency(total_latency / static_cast<double>(processed_transactions));
    return Status::OK;
}
