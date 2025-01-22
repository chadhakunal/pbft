#include "lib/workers/commit_worker.h"

void CommitWorker::process_request(CommitRequest req) {
    if(!is_server_faulty) {
        int received_view_number = req.view_number();
        int received_sequence_number = req.sequence_number();
        std::string message_digest = req.message_digest();
        int server_id = req.server_id();

        PrePrepareRequestState request_state = log_store.get_preprepare_request_state(received_view_number, received_sequence_number);

        if(received_view_number == server_state.get_current_view_number() && \
            log_store.is_digest_valid(received_view_number, received_sequence_number, message_digest)) {
            
            if(!server_state.is_leader() && !verify_signature(req)) {
                std::cout<<"Invalid Signature"<<std::endl;
                return;
            }

            log_store.add_commit_log(req);

            if(request_state == PrePrepareRequestState::Prepared) {
                if(log_store.commit_count(received_view_number, received_sequence_number) >= commit_majority_count) {
                    log_store.set_preprepare_request_state(received_view_number, received_sequence_number, PrePrepareRequestState::Committed);
                    
                    if(!is_server_faulty) {
                        CommittedRequest request;
                        *request.mutable_preprepare_request() = log_store.preprepare_request(received_view_number, received_sequence_number);

                        for (const auto& prepare_request : log_store.prepare_requests(received_view_number, received_sequence_number)) {
                            *request.add_prepare_requests() = prepare_request.second;
                        }

                        for (const auto& commit_request : log_store.commit_requests(received_view_number, received_sequence_number)) {
                            *request.add_commit_requests() = commit_request.second;
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
                    }
                }
            }
        } 
    }
}