#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <vector>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <thread>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
// #include <mcl/bn256.hpp>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "google/protobuf/empty.pb.h"
#include "linear-pbft.grpc.pb.h"
#include <google/protobuf/message.h>

extern bool is_server_running;
extern bool is_server_faulty;
extern int server_id;
extern int processed_transactions;
extern std::int64_t total_latency;
extern std::mutex performance_mutex;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern CryptoPP::RSA::PublicKey private_key;
extern std::vector<CryptoPP::RSA::PrivateKey> public_keys;

#pragma clang diagnostic pop

const int prepare_majority_count = 5;
const int commit_majority_count = 5;
const int initial_balance_amount = 10;
const int checkpoint_period = 10;
const int checkpoint_majority_count = 5;
const int view_change_majority_count = 5;
const int total_majority_count = 7;

enum PrePrepareRequestState {
    Init,
    Prepared,
    Committed,
    Executed,
    Invalidated,
};

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
            "linear-pbft-client:40060"
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
extern std::vector<std::unique_ptr<PbftClientService::Stub> > CLIENT_STUBS;

void initialize_global_variables(std::string host_id);

#endif
