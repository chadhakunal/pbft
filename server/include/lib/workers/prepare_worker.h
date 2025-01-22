#ifndef PREPARE_WORKER_H
#define PREPARE_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "lib/workers/prepared_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"

class PrepareWorker : public WorkerBase<PrepareRequest> {
private:
    ServerStateData& server_state;
    LogStore& log_store;
    PreparedWorker& prepared_worker;

    void process_request(PrepareRequest req) override;

    PrepareWorker(const PrepareWorker&) = delete;
    PrepareWorker& operator=(const PrepareWorker&) = delete;

public:
    PrepareWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), prepared_worker(PreparedWorker::get_instance()) {
        start_processing();
    }

    static PrepareWorker& get_instance() {
        static PrepareWorker instance;
        return instance;
    }
};

#endif