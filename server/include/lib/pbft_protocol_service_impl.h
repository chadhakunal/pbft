#ifndef PBFT_PROTOCOL_SERVICE_H
#define PBFT_PROTOCOL_SERVICE_H

#include "globals.h"
#include "lib/workers/commit_worker.h"
#include "lib/workers/execute_worker.h"
#include "lib/workers/prepare_worker.h"
#include "lib/workers/prepared_worker.h"
#include "lib/workers/preprepare_worker.h"
#include "lib/workers/request_worker.h"
#include "lib/workers/view_change_worker.h"
#include "lib/workers/new_view_worker.h"
#include "models/log_store.h"
#include "models/server_state_data.h"

#include <grpc/grpc.h>
#include <grpcpp/server_context.h>

using grpc::ServerContext;
using grpc::Status;

class PbftProtocolServiceImpl final: public PbftProtocolService::Service {
 private:
  PrePrepareWorker& preprepare_worker;
  PrepareWorker& prepare_worker;
  PreparedWorker& prepared_worker;
  ExecuteWorker& execute_worker;
  CommitWorker& commit_worker;
  ViewChangeWorker& view_change_worker;
  NewViewWorker& new_view_worker;

  ServerStateData& server_state;
  LogStore& log_store;
 public:
  PbftProtocolServiceImpl() : execute_worker(ExecuteWorker::get_instance()), commit_worker(CommitWorker::get_instance()), \
                              prepared_worker(PreparedWorker::get_instance()), prepare_worker(PrepareWorker::get_instance()), \
                              preprepare_worker(PrePrepareWorker::get_instance()), server_state(ServerStateData::get_instance()), \
                              log_store(LogStore::get_instance()), view_change_worker(ViewChangeWorker::get_instance()), \
                              new_view_worker(NewViewWorker::get_instance()) {};
  Status PrePrepare(ServerContext* context, const PrePrepareRequest* req, google::protobuf::Empty* res) override;
  Status Prepare(ServerContext* context, const PrepareRequest* req, google::protobuf::Empty* res) override;
  Status Prepared(ServerContext* context, const PreparedRequest* req, google::protobuf::Empty* res) override;
  Status Commit(ServerContext* context, const CommitRequest* req, google::protobuf::Empty* res) override;
  Status Committed(ServerContext* context, const CommittedRequest* req, google::protobuf::Empty* res) override;
  Status Catchup(ServerContext* context, const CatchupRequest* req, CatchupResponse* res) override;

  Status Checkpoint(ServerContext* context, const CheckpointRequest* req, google::protobuf::Empty* res) override;
  Status ViewChange(ServerContext* context, const ViewChangeRequest* req, google::protobuf::Empty* res) override;
  Status NewView(ServerContext* context, const NewViewRequest* req, google::protobuf::Empty* res) override;
};

#endif
