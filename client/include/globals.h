#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <iomanip>
#include <vector>
#include <queue>
#include <utility>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include <chrono>
#include <cstdint>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server_builder.h>
#include <boost/algorithm/string.hpp>
#include "google/protobuf/empty.pb.h"
#include "linear-pbft.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

const std::string TEST_CASE_PATH = "../tests/test-1.csv";
const int reply_majority_count = 5;

const std::vector<std::string> SERVER_HOSTS = [] {
    const char* running_in_docker = std::getenv("DOCKER_ENV");
    if (running_in_docker && std::string(running_in_docker) == "true") {
        return std::vector<std::string>{
            "linear-pbft-server_1:50051",
            "linear-pbft-server_2:50052",
            "linear-pbft-server_3:50053",
            "linear-pbft-server_4:50054",
            "linear-pbft-server_5:50055",
            "linear-pbft-server_6:50056",
            "linear-pbft-server_7:50057"
        };
    } else {
        return std::vector<std::string>{
            "0.0.0.0:50051",
            "0.0.0.0:50052",
            "0.0.0.0:50053",
            "0.0.0.0:50054",
            "0.0.0.0:50055",
            "0.0.0.0:50056",
            "0.0.0.0:50057"
        };
    }
}();

const std::vector<std::string> CLIENT_HOSTS = [] {
    const char* running_in_docker = std::getenv("DOCKER_ENV");
    if (running_in_docker && std::string(running_in_docker) == "true") {
        return std::vector<std::string>{
            "linear-pbft-client:40051",
            "linear-pbft-client:40052",
            "linear-pbft-client:40053",
            "linear-pbft-client:40054",
            "linear-pbft-client:40055",
            "linear-pbft-client:40056",
            "linear-pbft-client:40057",
            "linear-pbft-client:40058",
            "linear-pbft-client:40059",
            "linear-pbft-client:40060",
        };
    } else {
        return std::vector<std::string>{
            "0.0.0.0:40051",
            "0.0.0.0:40052",
            "0.0.0.0:40053",
            "0.0.0.0:40054",
            "0.0.0.0:40055",
            "0.0.0.0:40056",
            "0.0.0.0:40057",
            "0.0.0.0:40058",
            "0.0.0.0:40059",
            "0.0.0.0:40060"
        };
    }
}();

extern std::vector<std::unique_ptr<PbftProtocolService::Stub> > SERVER_STUBS;

#endif
