#include "lib/workers/new_view_worker.h"

void NewViewWorker::process_request(NewViewRequest req) {
    server_state.update_state({{"current_view_number", std::to_string(req.view_number())}, {"view_change", "false"}});
    log_store.add_new_view_request(req);
    // std::cout<<"NEW VIEW RECEIVED FROM: "<<req.server_id()<<", "<<req.view_number()<<", "<<req.preprepare_requests().size()<<std::endl;

    int next_available_seq = 1;
    for (const auto& preprepare_request : req.preprepare_requests()) {
        next_available_seq = std::max(next_available_seq, preprepare_request.sequence_number());
    }

    server_state.update_state({{"available_sequence_number", std::to_string(next_available_seq)}});
    
    for (const auto& preprepare_request : req.preprepare_requests()) {
        preprepare_worker.push_request(preprepare_request);
    }
}
