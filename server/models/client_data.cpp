#include "models/client_data.h"

bool ClientData::execute_transaction(int from, int to, int amount) {
    std::lock_guard<std::mutex> lock(balance_mutex);
    if(balances[from] - amount < 0) {
        return false;
    }

    balances[from] -= amount;
    balances[to] += amount;
    return true;
}

void ClientData::undo_transaction(int from, int to, int amount) {
    std::lock_guard<std::mutex> lock(balance_mutex);
    balances[to] -= amount;
    balances[from] += amount;
}

std::unordered_map<int, int>& ClientData::get_all_balances() {
    std::lock_guard<std::mutex> lock(balance_mutex);
    return balances;
}

std::string ClientData::hash_balances() {
    std::lock_guard<std::mutex> lock(balance_mutex);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    for (const auto& [client_id, balance] : balances) {
        std::string entry = std::to_string(client_id) + ":" + std::to_string(balance) + ";";
        SHA256_Update(&sha256, entry.data(), entry.size());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::ostringstream oss;
    for (unsigned char byte : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    return oss.str();
}

void ClientData::reset_balances() {
    std::lock_guard<std::mutex> lock(balance_mutex);
    for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
        balances[i] = initial_balance_amount;
    }
}