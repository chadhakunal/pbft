#ifndef NEW_VIEW_WORKER_H
#define NEW_VIEW_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "lib/workers/preprepare_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"

class NewViewWorker : public WorkerBase<NewViewRequest> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    PrePrepareWorker& preprepare_worker;
    void process_request(NewViewRequest req) override;

    NewViewWorker(const NewViewWorker&) = delete;
    NewViewWorker& operator=(const NewViewWorker&) = delete;

public:
    NewViewWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), preprepare_worker(PrePrepareWorker::get_instance()) {
        start_processing();
    }

    static NewViewWorker& get_instance() {
        static NewViewWorker instance;
        return instance;
    }
};

#endif