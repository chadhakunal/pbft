#ifndef VIEW_CHANGE_WORKER_H
#define VIEW_CHANGE_WORKER_H

#include "globals.h"
#include "utils.h"
#include "lib/workers/worker_base.h"
#include "lib/workers/new_view_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"

class ViewChangeWorker : public WorkerBase<ViewChangeRequest> {
private:
    LogStore& log_store;
    ServerStateData& server_state;
    NewViewWorker& new_view_worker;
    void process_request(ViewChangeRequest req) override;

    ViewChangeWorker(const ViewChangeWorker&) = delete;
    ViewChangeWorker& operator=(const ViewChangeWorker&) = delete;

public:
    ViewChangeWorker() : server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), new_view_worker(NewViewWorker::get_instance()) {
        start_processing();
    }

    static ViewChangeWorker& get_instance() {
        static ViewChangeWorker instance;
        return instance;
    }
};

#endif