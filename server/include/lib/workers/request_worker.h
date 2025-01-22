#ifndef REQUEST_WORKER_H
#define REQUEST_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "lib/workers/preprepare_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"

class RequestWorker : public WorkerBase<TransactionRequest> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    PrePrepareWorker& preprepare_worker;

    RequestWorker(const RequestWorker&) = delete;
    RequestWorker& operator=(const RequestWorker&) = delete;

    void process_request(TransactionRequest req) override;
public:
    RequestWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), preprepare_worker(PrePrepareWorker::get_instance()) {
        start_processing();
    }

    static RequestWorker& get_instance() {
        static RequestWorker instance;
        return instance;
    }
};

#endif