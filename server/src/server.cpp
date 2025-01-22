#include "server.h"

std::unique_ptr<grpc::Server> server;

void run_server(std::string& port) {
    std::string server_address("0.0.0.0:" + port);
    ServerBuilder builder;

    PbftClientServiceImpl pbft_client_service;
    PbftProtocolServiceImpl pbft_protocol_service;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&pbft_client_service);
    builder.RegisterService(&pbft_protocol_service);
    server = builder.BuildAndStart();

    std::cout<<"Server listening on "<<server_address<<std::endl;
    server->Wait();
}

void handleSignal(int signum) {
    std::cout<<"Terminating Server!"<<std::endl;
    if (server) {
        server->Shutdown();  // Initiates graceful shutdown
    }
}

int main(int argc, char** argv) {
    const char* host_id_env = getenv("HOST_ID");
    const char* port_env = getenv("PORT");
    
    if(host_id_env == nullptr) {
        std::cout<<"Env Variable `HOST_ID` missing!"<<std::endl;
        return 1;
    }

    if(port_env == nullptr) {
        std::cout<<"Env Variable `PORT` missing!"<<std::endl;
        return 1;
    }

    // mcl::bn256::initPairing();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    initialize_global_variables(std::string(host_id_env));

    std::string host_id = std::string(host_id_env);
    std::string port = (port_env != nullptr) ? std::string(port_env) : "50051";
    server_id = std::stoi(host_id);

    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    run_server(port);
    return 0;
}
