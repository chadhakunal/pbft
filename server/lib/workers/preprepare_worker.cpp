#include "lib/workers/preprepare_worker.h"

void PrePrepareWorker::process_request(PrePrepareRequest req) {
    int received_view_number = req.view_number();
    int received_sequence_number = req.sequence_number();
    std::string message_digest = req.message_digest();
    TransactionRequestEntry message = req.message();

    if(req.is_nop()) {
        if(!log_store.preprepare_exists(received_view_number, received_sequence_number)) {
            log_store.add_preprepare_log(req);
        }
        log_store.set_preprepare_request_state(received_view_number, received_sequence_number, PrePrepareRequestState::Executed);
        return;
    }

    std::int64_t timestamp = message.timestamp();
    int client_id = message.client_id();
    if(!log_store.transaction_reply_exists(client_id, timestamp)) {
        log_store.add_transaction_reply(client_id, timestamp);
    }

    if(received_view_number == server_state.get_current_view_number() && hash_transaction(message) == message_digest)  {
        if(!server_state.is_leader() && !verify_signature(req)) {
            return;
        }

        
        if(!log_store.preprepare_exists(received_view_number, received_sequence_number)) {
            log_store.add_preprepare_log(req);
            server_state.update_state({{"last_received_sequence", std::to_string(received_sequence_number)}});

            if(!is_server_faulty) {
                PrepareRequest prepare_request;
                prepare_request.set_view_number(received_view_number);
                prepare_request.set_sequence_number(received_sequence_number);
                prepare_request.set_message_digest(message_digest);
                prepare_request.set_server_id(server_id);
                set_signature(prepare_request);

                if(server_state.is_leader()) {
                    prepare_worker.push_request(prepare_request);
                } else {
                    log_store.add_prepare_log(prepare_request);
                    grpc::ClientContext context;
                    google::protobuf::Empty response;
                    int leader_id = server_state.get_current_view_number() % SERVER_HOSTS.size();
                    grpc::Status status = SERVER_STUBS[leader_id]->Prepare(&context, prepare_request, &response);

                    if (!status.ok()) {
                        std::cerr << "Prepare RPC failed: " << status.error_message() << std::endl;
                    }   
                }
            }
        }
    }
}