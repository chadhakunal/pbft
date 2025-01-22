#include "lib/workers/prepared_worker.h"

void PreparedWorker::process_request(PreparedRequest req) {
    if(!is_server_faulty) {
        const PrePrepareRequest& preprepare = req.preprepare_request();
        int preprepare_view_number = preprepare.view_number();
        int preprepare_sequence_number = preprepare.sequence_number();
        std::string preprepare_message_digest = preprepare.message_digest();
        TransactionRequestEntry preprepare_message = preprepare.message();

        if(!server_state.is_leader()) {
            if(!log_store.preprepare_exists(preprepare_view_number, preprepare_sequence_number)) {
                if(preprepare_view_number == server_state.get_current_view_number() && hash_transaction(preprepare_message) == preprepare_message_digest)  {
                    log_store.add_preprepare_log(preprepare);
                    if(!log_store.transaction_reply_exists(preprepare_message.client_id(), preprepare_message.timestamp())) {
                        log_store.add_transaction_reply(preprepare_message.client_id(), preprepare_message.timestamp());
                    }
                    server_state.update_state({{"last_received_sequence", std::to_string(preprepare_sequence_number)}});
                }
            }

            for (const auto& prepare_request : req.prepare_requests()) {
                int prepare_view_number = prepare_request.view_number();
                int prepare_sequence_number = prepare_request.sequence_number();
                std::string prepare_message_digest = prepare_request.message_digest();
                
                if(prepare_view_number == server_state.get_current_view_number() && log_store.is_digest_valid(prepare_view_number, prepare_sequence_number, prepare_message_digest)) {
                    if(verify_signature(prepare_request)) {
                        log_store.add_prepare_log(prepare_request);
                        if(log_store.get_preprepare_request_state(prepare_view_number, prepare_sequence_number) == PrePrepareRequestState::Init) {
                            if(log_store.prepare_count(prepare_view_number, prepare_sequence_number) >= prepare_majority_count) {
                                log_store.set_preprepare_request_state(prepare_view_number, prepare_sequence_number, PrePrepareRequestState::Prepared);
                            }
                        }
                    }
                }
            }
        }

        if(log_store.get_preprepare_request_state(preprepare_view_number, preprepare_sequence_number) == PrePrepareRequestState::Prepared) {
            if(!is_server_faulty) {
                CommitRequest commit_request;
                commit_request.set_view_number(preprepare_view_number);
                commit_request.set_sequence_number(preprepare_sequence_number);
                commit_request.set_message_digest(preprepare_message_digest);
                commit_request.set_server_id(server_id);
                set_signature(commit_request);

                if(server_state.is_leader()) {
                    commit_worker.push_request(commit_request);
                } else {
                    grpc::ClientContext context;
                    google::protobuf::Empty response;
                    int leader_id = server_state.get_current_view_number() % SERVER_HOSTS.size();
                    grpc::Status status = SERVER_STUBS[leader_id]->Commit(&context, commit_request, &response);

                    if (!status.ok()) {
                        std::cerr << "Commit RPC failed: " << status.error_message() << std::endl;
                    }
                }
            }
        }
    }
}
