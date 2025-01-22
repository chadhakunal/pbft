#ifndef PREPREPARE_WORKER_H
#define PREPREPARE_WORKER_H

#include "globals.h"
#include "utils.h"

#include "lib/workers/worker_base.h"
#include "models/server_state_data.h"
#include "lib/workers/prepare_worker.h"
#include "models/log_store.h"

class PrePrepareWorker : public WorkerBase<PrePrepareRequest> {
private:
    ServerStateData& server_state;
    LogStore& log_store;
    PrepareWorker &prepare_worker;

    void process_request(PrePrepareRequest req) override;

    PrePrepareWorker(const PrePrepareWorker&) = delete;
    PrePrepareWorker& operator=(const PrePrepareWorker&) = delete;

public:
    PrePrepareWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), prepare_worker(PrepareWorker::get_instance()) {
        start_processing();
    }

    static PrePrepareWorker& get_instance() {
        static PrePrepareWorker instance;
        return instance;
    }
};

#endif