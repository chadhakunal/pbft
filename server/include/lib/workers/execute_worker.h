#ifndef EXECUTE_WORKER_H
#define EXECUTE_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/pq_worker_base.h"
#include "models/server_state_data.h"
#include "models/log_store.h"
#include "models/client_data.h"

struct CommittedRequestComparator {
    bool operator()(CommittedRequest lhs, CommittedRequest rhs) const {
        if (lhs.preprepare_request().view_number() != rhs.preprepare_request().view_number()) {
            return lhs.preprepare_request().view_number() > rhs.preprepare_request().view_number();
        }
        
        return lhs.preprepare_request().sequence_number() < rhs.preprepare_request().sequence_number();
    }
};

class ExecuteWorker : public PriorityQueueWorkerBase<CommittedRequest, CommittedRequestComparator> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    ClientData& client_data;
    void process_request(CommittedRequest req) override;

    ExecuteWorker(const ExecuteWorker&) = delete;
    ExecuteWorker& operator=(const ExecuteWorker&) = delete;

public:
    ExecuteWorker() 
        : PriorityQueueWorkerBase<CommittedRequest, CommittedRequestComparator>(),
          server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), client_data(ClientData::get_instance()) {
        start_processing();
    }

    bool sequence_number_exists_in_queue(int seq_num);

    static ExecuteWorker& get_instance() {
        static ExecuteWorker instance;
        return instance;
    }
};

#endif
