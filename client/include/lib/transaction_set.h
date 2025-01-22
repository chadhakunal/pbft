#ifndef TRANSACTION_SET_H
#define TRANSACTION_SET_H

#include "globals.h"

#include "lib/pbft_client.h"

class TransactionSet {
 private:
   std::string set_id;
   std::vector<std::vector<int>> transactions;
   std::vector<bool> live_servers;
   std::vector<bool> byzantine_servers;

 public:
   TransactionSet(std::string set, std::vector<std::string> live_servers_list, std::vector<std::string> byzantine_servers_list);
   std::vector<bool> get_live_servers();
   std::vector<bool> get_byzantine_servers();
   std::vector<std::vector<int>> get_transactions();
   std::string get_set_id();
   void addTransaction(const std::string& transaction_str);
};

#endif
