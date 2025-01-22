#include "lib/pbft_protocol_service_impl.h"

Status PbftProtocolServiceImpl::PrePrepare(ServerContext* context, const PrePrepareRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        if(!server_state.in_view_change()) {
            preprepare_worker.push_request(*req);
        }
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Prepare(ServerContext* context, const PrepareRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        if(!server_state.in_view_change()) {
            prepare_worker.push_request(*req);  
        }
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Prepared(ServerContext* context, const PreparedRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        if(!server_state.in_view_change()) {
            prepared_worker.push_request(*req);  
        }
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Commit(ServerContext* context, const CommitRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        if(!server_state.in_view_change()) {
            commit_worker.push_request(*req);  
        }
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Committed(ServerContext* context, const CommittedRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        if(!server_state.in_view_change()) {
            execute_worker.push_request(*req);  
        }
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Catchup(ServerContext* context, const CatchupRequest* req, CatchupResponse* res) {
    std::cout<<"Catchup Received!"<<std::endl;
    if(is_server_running) {
        int view_number = req->view_number();
        int from_sequence_number = req->from_sequence_number();
        int to_sequence_number = server_state.get_last_executed_sequence();

        for(int seq_num = from_sequence_number; seq_num <= to_sequence_number; seq_num++) {        
            if(log_store.preprepare_exists(view_number, seq_num)) {
                CommittedRequest* request = res->add_committed_requests();
                *request->mutable_preprepare_request() = log_store.preprepare_request(view_number, seq_num);

                for (const auto& prepare_request : log_store.prepare_requests(view_number, seq_num)) {
                    *request->add_prepare_requests() = prepare_request.second;
                }

                for (const auto& commit_request : log_store.commit_requests(view_number, seq_num)) {
                    *request->add_commit_requests() = commit_request.second;
                }
            }
        }
        
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::Checkpoint(ServerContext* context, const CheckpointRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        log_store.add_checkpoint_request(*req);
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::ViewChange(ServerContext* context, const ViewChangeRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        view_change_worker.push_request(*req);
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}

Status PbftProtocolServiceImpl::NewView(ServerContext* context, const NewViewRequest* req, google::protobuf::Empty* res) {
    if(is_server_running) {
        new_view_worker.push_request(*req);
        return Status::OK;
    }
    return Status(grpc::StatusCode::UNAVAILABLE, "Server is temporarily unavailable.");
}
