#include "lib/transaction_set.h"

TransactionSet::TransactionSet(std::string set, std::vector<std::string> live_servers_list, std::vector<std::string> byzantine_servers_list) : set_id(set), live_servers(7, false), byzantine_servers(7, false) {
    for (const std::string server_str : live_servers_list) {
        if(server_str != "") {
            int server = std::stoi(server_str);
            live_servers[server - 1] = true;
        }
    }

    for (const std::string server_str : byzantine_servers_list) {
        if(server_str != "") {
            int server = std::stoi(server_str);
            byzantine_servers[server - 1] = true;
        }
    }
}

std::vector<bool> TransactionSet::get_live_servers() {
    return live_servers;
}

std::vector<bool> TransactionSet::get_byzantine_servers() {
    return byzantine_servers;
}

std::vector<std::vector<int>> TransactionSet::get_transactions() {
    return transactions;
}

std::string TransactionSet::get_set_id() {
    return set_id;
}

void TransactionSet::addTransaction(const std::string& transaction_str) {
    std::vector<std::string> transaction_str_list;
    std::vector<int> transaction(3);
    boost::split(transaction_str_list, transaction_str, boost::is_any_of(","), boost::token_compress_on);

    for (auto& t : transaction_str_list) {
        boost::trim(t);
    }

    transaction[0] = static_cast<int>(transaction_str_list[0][0] - 'A');
    transaction[1] = static_cast<int>(transaction_str_list[1][0] - 'A');
    transaction[2] = std::stoi(transaction_str_list[2]);
    transactions.push_back(transaction);
}
