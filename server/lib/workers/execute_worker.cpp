#include "lib/workers/execute_worker.h"

void ExecuteWorker::process_request(CommittedRequest req) {
    if(!is_server_faulty) {
        const PrePrepareRequest& preprepare = req.preprepare_request();
        int preprepare_view_number = preprepare.view_number();
        int preprepare_sequence_number = preprepare.sequence_number();
        std::string preprepare_message_digest = preprepare.message_digest();
        TransactionRequestEntry preprepare_message = preprepare.message();
        int client_id = preprepare_message.client_id();

        if(!log_store.preprepare_exists(preprepare_view_number, preprepare_sequence_number)) {
            if(preprepare_view_number == server_state.get_current_view_number() && hash_transaction(preprepare_message) == preprepare_message_digest)  {
                log_store.add_preprepare_log(preprepare);
                if(!log_store.transaction_reply_exists(preprepare_message.client_id(), preprepare_message.timestamp())) {
                    log_store.add_transaction_reply(preprepare_message.client_id(), preprepare_message.timestamp());
                }
                server_state.update_state({{"last_received_sequence", std::to_string(preprepare_sequence_number)}});
            }
        }

        if (log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Executed) {
            return;
        }

        if (log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Init) {
            for (const auto& prepare_request : req.prepare_requests()) {
                int prepare_view_number = prepare_request.view_number();
                int prepare_sequence_number = prepare_request.sequence_number();
                std::string prepare_message_digest = prepare_request.message_digest();

                if(prepare_view_number == server_state.get_current_view_number() && log_store.is_digest_valid(prepare_view_number, prepare_sequence_number, prepare_message_digest) && verify_signature(prepare_request)) {
                    log_store.add_prepare_log(prepare_request);
                    if(log_store.get_preprepare_request_state(prepare_view_number, prepare_sequence_number) == PrePrepareRequestState::Init) {
                        if(log_store.prepare_count(prepare_view_number, prepare_sequence_number) >= prepare_majority_count) {
                            log_store.set_preprepare_request_state(prepare_view_number, prepare_sequence_number, PrePrepareRequestState::Prepared);
                        }
                    } else if(log_store.get_preprepare_request_state(prepare_view_number, prepare_sequence_number) == PrePrepareRequestState::Prepared) {
                        if(log_store.prepare_count(prepare_view_number, prepare_sequence_number) == total_majority_count) {
                            log_store.set_preprepare_request_state(prepare_view_number, prepare_sequence_number, PrePrepareRequestState::Committed);
                        }
                    }
                }
            }
        }

        if (log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Prepared) {
            for (const auto& commit_request : req.commit_requests()) {
                int commit_view_number = commit_request.view_number();
                int commit_sequence_number = commit_request.sequence_number();
                std::string commit_message_digest = commit_request.message_digest();

                if(commit_view_number == server_state.get_current_view_number() && log_store.is_digest_valid(commit_view_number, commit_sequence_number, commit_message_digest) && verify_signature(commit_request)) {
                    log_store.add_commit_log(commit_request);
                    if(log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Prepared) {
                        if(log_store.commit_count(commit_view_number, commit_sequence_number) >= commit_majority_count) {
                            log_store.set_preprepare_request_state(commit_view_number, commit_sequence_number, PrePrepareRequestState::Committed);
                        }
                    }
                }
            }
        }

        if (log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Committed) {
            if(preprepare_sequence_number == server_state.get_last_executed_sequence() + 1 || log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number - 1) == PrePrepareRequestState::Executed) {
                bool result = client_data.execute_transaction(preprepare_message.sender(), preprepare_message.receiver(), preprepare_message.amount());        
                TransactionReplyRequest client_reply_request;

                client_reply_request.set_view_number(preprepare_view_number);
                client_reply_request.set_timestamp(preprepare_message.timestamp());
                client_reply_request.set_client_id(preprepare_message.client_id());
                client_reply_request.set_server_id(server_id);
                client_reply_request.set_is_invalid_request(result);

                const auto& balances = client_data.get_all_balances();
                std::string balances_data;

                for (const auto& entry : balances) {
                    balances_data += std::to_string(entry.first) + ":" + std::to_string(entry.second) + ";";
                }

                unsigned char hash[SHA256_DIGEST_LENGTH];
                SHA256(reinterpret_cast<const unsigned char*>(balances_data.c_str()), balances_data.size(), hash);

                std::string hash_hex;
                for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02x", hash[i]);
                    hash_hex += buf;
                }
                client_reply_request.set_balances_digest(hash_hex);
                set_signature(client_reply_request);
                
                log_store.transaction_reply_completed(preprepare_message.client_id(), preprepare_message.timestamp(), client_reply_request);
                log_store.set_preprepare_request_state(preprepare_view_number, preprepare_sequence_number, PrePrepareRequestState::Executed);
                server_state.update_state({{"last_executed_sequence", std::to_string(preprepare_sequence_number)}});
                {
                    std::lock_guard<std::mutex> lock(performance_mutex);
                    processed_transactions++;
                    std::int64_t current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    total_latency += current_timestamp - log_store.get_preprepare_request_creation_timestamp(preprepare_view_number, preprepare_sequence_number);
                }

                std::thread([client_id, client_reply_request]() {
                    int max_retries = 0;
                    int retry_count = 0;
                    int initial_delay_ms = 100;

                    google::protobuf::Empty response;
                    grpc::ClientContext context;

                    grpc::Status status = CLIENT_STUBS[client_id]->TransactionReply(&context, client_reply_request, &response);
                    return;
                }).detach(); 

                if(preprepare_sequence_number % checkpoint_period == 0) {
                    CheckpointRequest checkpoint_request;
                    checkpoint_request.set_view_number(preprepare_view_number);
                    checkpoint_request.set_sequence_number(preprepare_sequence_number);
                    checkpoint_request.set_server_id(server_id);
                    checkpoint_request.set_state_digest(client_data.hash_balances());
                    for (const auto& entry : client_data.get_all_balances()) {
                        BalanceEntry* balance_entry = checkpoint_request.add_state();
                        balance_entry->set_client_id(entry.first);
                        balance_entry->set_amount(entry.second);
                    }

                    std::thread([checkpoint_request]() {
                        LogStore &ls = LogStore::get_instance();
                        ls.add_checkpoint_request(checkpoint_request);

                        for (const auto& server_stub : SERVER_STUBS) {
                            if (server_stub) {
                                grpc::ClientContext context;
                                google::protobuf::Empty response;
                                grpc::Status status = server_stub->Checkpoint(&context, checkpoint_request, &response);

                                if (!status.ok()) {
                                    std::cerr << "Checkpoint RPC failed: " << status.error_message() << std::endl;
                                }
                            }
                        }
                    }).detach();
                }
            } else {
                if(server_state.is_leader()) {
                    // std::cout<<"Delaying and Waiting!"<<preprepare_sequence_number<<", "<<server_state.get_last_executed_sequence()<<std::endl;
                    std::thread([req] {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        ExecuteWorker& e_worker = ExecuteWorker::get_instance();
                        e_worker.push_request(req);
                    }).detach();
                } else {
                    // std::cout<<"Backup Delaying and Waiting!"<<preprepare_sequence_number<<", "<<server_state.get_last_executed_sequence()<<std::endl;
                    std::thread([req] {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        ExecuteWorker& e_worker = ExecuteWorker::get_instance();
                        e_worker.push_request(req);
                    }).detach();
                }  
            }
        }
    }
}
