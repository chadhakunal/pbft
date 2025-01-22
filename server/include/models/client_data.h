#ifndef MODELS_CLIENT_DATA
#define MODELS_CLIENT_DATA

#include "globals.h"

class ClientData {
 private:
    std::unordered_map<int, int> balances;
    std::mutex balance_mutex;
    
    ClientData() {
        reset_balances();
    }

 public:
    ClientData(const ClientData&) = delete;
    ClientData& operator=(const ClientData&) = delete;

    static ClientData& get_instance() {
        static ClientData instance;
        return instance;
    }

    bool execute_transaction(int from, int to, int amount);
    void undo_transaction(int from, int to, int amount);
    std::unordered_map<int, int>& get_all_balances();

    std::string hash_balances();
    void reset_balances();
};

#endif
