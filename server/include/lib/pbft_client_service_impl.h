#ifndef PBFT_CLIENT_SERVICE_H
#define PBFT_CLIENT_SERVICE_H

#include "globals.h"
#include "lib/workers/request_worker.h"
#include "lib/workers/view_change_worker.h"
#include "models/server_state_data.h"
#include "models/log_store.h"
#include "models/client_data.h"

#include <grpc/grpc.h>
#include <grpcpp/server_context.h>

using grpc::ServerContext;
using grpc::Status;

class PbftClientServiceImpl final: public PbftClientService::Service {
 private:
   RequestWorker& request_worker;
   ViewChangeWorker& view_change_worker;
   ServerStateData& server_state;
   LogStore& log_store;
   ClientData& client_data;

 public:
    PbftClientServiceImpl() : client_data(ClientData::get_instance()), server_state(ServerStateData::get_instance()), log_store(LogStore::get_instance()), request_worker(RequestWorker::get_instance()), view_change_worker(ViewChangeWorker::get_instance()) {}

    Status SendTransaction(ServerContext* context, const TransactionRequest* req, TransactionResponse* res) override;
    
    Status PrintLog(ServerContext* context, const google::protobuf::Empty* req, PrintLogResponse* res) override;
    Status PrintDB(ServerContext* context, const google::protobuf::Empty* req, PrintDBResponse* res) override;
    Status PrintStatus(ServerContext* context, const PrintStatusRequest* req, PrintStatusResponse* res) override;
    Status PrintView(ServerContext* context, const google::protobuf::Empty* req, PrintViewResponse* res) override;
    Status Performance(ServerContext* context, const google::protobuf::Empty* req, PerformanceResponse* res) override;

    Status ServerUp(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) override;
    Status ServerDown(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) override;
    Status MakeFaulty(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) override;
    Status MakeNonFaulty(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) override;
    Status ResetServer(ServerContext* context, const google::protobuf::Empty* req, google::protobuf::Empty* res) override;
};

#endif
