#ifndef PBFT_SERVER_H
#define PBFT_SERVER_H

#include "globals.h"

class PbftServerInterface {
 private:
  std::unique_ptr<PbftClientService::Stub> stub_;
  int server_id;

 public:
  PbftServerInterface(int id, std::shared_ptr<Channel> channel);
  std::string get_server_id_string();

  void server_up();
  void server_down();
  void make_faulty();
  void make_non_faulty();
  void reset_server();

  PrintLogResponse get_logs();
  PrintViewResponse get_new_view_requests();
  std::string get_status(int sequence_number);
  PrintDBResponse get_db();
  std::vector<double> get_performance();

  std::unique_ptr<TransactionResponse> send_transaction(std::int64_t timestamp, std::vector<int> transaction, int client_id, bool retrying = false);
};

#endif
