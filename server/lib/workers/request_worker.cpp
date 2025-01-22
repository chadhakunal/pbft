#include "lib/workers/request_worker.h"

void RequestWorker::process_request(TransactionRequest req) {
    int sequence_num = server_state.get_available_sequence_number();
    server_state.update_state({{"available_sequence_number", std::to_string(sequence_num + 1)}});
    
    PrePrepareRequest preprepare_request;
    preprepare_request.set_view_number(server_state.get_current_view_number());
    preprepare_request.set_sequence_number(sequence_num);
    preprepare_request.set_server_id(server_id);
    preprepare_request.set_message_digest(hash_transaction(req.transaction()));

    *preprepare_request.mutable_message() = req.transaction();
    set_signature(preprepare_request);
    preprepare_worker.push_request(preprepare_request);
    
    for (const auto& server_stub : SERVER_STUBS) {
        if (server_stub) {
            grpc::ClientContext context;
            google::protobuf::Empty response;
            grpc::Status status = server_stub->PrePrepare(&context, preprepare_request, &response);

            if (!status.ok()) {
                std::cerr << "PrePrepare RPC failed: " << status.error_message() << std::endl;
            }
        }
    }
}
