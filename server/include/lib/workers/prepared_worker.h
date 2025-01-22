#ifndef PREPARED_WORKER_H
#define PREPARED_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "models/server_state_data.h"
#include "lib/workers/commit_worker.h"
#include "models/log_store.h"

class PreparedWorker : public WorkerBase<PreparedRequest> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    CommitWorker& commit_worker;

    void process_request(PreparedRequest req) override;

    PreparedWorker(const PreparedWorker&) = delete;
    PreparedWorker& operator=(const PreparedWorker&) = delete;

public:
    PreparedWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), commit_worker(CommitWorker::get_instance()) {
        start_processing();
    }

    static PreparedWorker& get_instance() {
        static PreparedWorker instance;
        return instance;
    }
};

#endif