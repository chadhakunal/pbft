#ifndef COMMIT_WORKER_H
#define COMMIT_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "lib/workers/execute_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"

class CommitWorker : public WorkerBase<CommitRequest> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    ExecuteWorker& execute_worker;
    void process_request(CommitRequest req) override;

    CommitWorker(const CommitWorker&) = delete;
    CommitWorker& operator=(const CommitWorker&) = delete;

public:
    CommitWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), execute_worker(ExecuteWorker::get_instance()) {
        start_processing();
    }

    static CommitWorker& get_instance() {
        static CommitWorker instance;
        return instance;
    }
};

#endif