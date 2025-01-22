#ifndef PBFT_CLIENT_H
#define PBFT_CLIENT_H

#include "globals.h"
#include "lib/server_interface.h"

class PbftClient final: public PbftClientService::Service {
 private:
  std::vector<std::unique_ptr<PbftClientService::Stub> > server_stubs_;
  int client_id;
  int leader_id;
  std::int64_t current_transaction_ts;

  std::vector<std::unique_ptr<PbftServerInterface>>& servers;

  std::queue<std::vector<int> > transaction_queue;
  std::mutex queue_mutex;
  std::condition_variable queue_condition;

  std::thread processing_thread;
  bool stop_processing;

  std::unordered_map<int, TransactionReplyRequest> transaction_replies;
  std::mutex transaction_replies_mutex;
  std::condition_variable transaction_replies_condition;

  void process_transactions();

 public:
  PbftClient(int id, std::vector<std::unique_ptr<PbftServerInterface>>& servers);
  ~PbftClient();
  
  inline char get_client_id_string();
  
  void push_transaction(std::vector<int> transaction);

  bool verify_signature(std::string signature, int server_id) {
      return true;
  }

  void reset() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_processing = true;
        queue_condition.notify_all();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    if (processing_thread.joinable()) {
        processing_thread.join();
    }

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        while (!transaction_queue.empty()) {
            transaction_queue.pop();
        }
    }

    {
        std::unique_lock<std::mutex> lock(transaction_replies_mutex);
        transaction_replies.clear();
    }
    
    stop_processing = false;
    processing_thread = std::thread(&PbftClient::process_transactions, this);
  }

  Status TransactionReply(ServerContext* context, const TransactionReplyRequest* req, google::protobuf::Empty* res) override;
};

#endif
