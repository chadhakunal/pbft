#include "globals.h"

bool is_server_running = true;
bool is_server_faulty = false;

int server_id = -1;

int processed_transactions = 0;
std::int64_t total_latency = 0;
std::mutex performance_mutex;

CryptoPP::RSA::PublicKey private_key;
std::vector<CryptoPP::RSA::PrivateKey> public_keys;

std::vector<std::unique_ptr<PbftProtocolService::Stub> > SERVER_STUBS {};
std::vector<std::unique_ptr<PbftClientService::Stub> > CLIENT_STUBS {};

void initialize_global_variables(std::string host_id) {
    server_id = std::stoi(host_id);
    for(int i = 0; i < SERVER_HOSTS.size(); i++) {
        if(i != std::stoi(host_id)) {
            SERVER_STUBS.push_back(PbftProtocolService::NewStub(grpc::CreateChannel(SERVER_HOSTS[i], grpc::InsecureChannelCredentials())));
        } else {
            SERVER_STUBS.push_back(nullptr);
        }
    }


    for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
        CLIENT_STUBS.push_back(PbftClientService::NewStub(grpc::CreateChannel(CLIENT_HOSTS[i], grpc::InsecureChannelCredentials())));
    }


    std::string private_key_path = "../keys/private_key_" + host_id + ".der";
    try {
        CryptoPP::FileSource private_key_file(private_key_path.c_str(), true);
        private_key.Load(private_key_file);
        // std::cout << "Private key loaded successfully!" << std::endl;
    } catch (const CryptoPP::Exception& e) {
        std::cerr << "Error loading private key: " << e.what() << std::endl;
    }


    // Load the public keys for other servers
    for (int i = 0; i < SERVER_HOSTS.size(); i++) {
        CryptoPP::RSA::PrivateKey public_key;
        if (server_id != i) {
            std::string public_key_path = "../keys/public_key_" + std::to_string(i) + ".der";

            try {
                CryptoPP::FileSource public_key_file(public_key_path.c_str(), true);
                public_key.Load(public_key_file);
                // std::cout << "Public key loaded successfully!" << std::endl;
            } catch (const CryptoPP::Exception& e) {
                std::cerr << "Error loading public key: " << e.what() << std::endl;
            }   
        }
        public_keys.push_back(public_key);
    }
}
