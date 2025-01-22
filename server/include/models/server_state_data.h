#ifndef MODELS_SERVER_STATE_DATA
#define MODELS_SERVER_STATE_DATA

#include "globals.h"

class ServerStateData {
 private:
  mutable std::shared_mutex mtx;

  static int current_view_number;
  static int available_sequence_number;
  static int last_executed_sequence;
  static bool view_change;
  static int last_received_sequence;
  
  ServerStateData();

  ServerStateData(const ServerStateData&) = delete;
  ServerStateData& operator=(const ServerStateData&) = delete;

 public:
  static ServerStateData& get_instance() {
    static ServerStateData instance;
    return instance;
  }

  int update_state(const std::unordered_map<std::string, std::string>& new_state);
  int get_current_view_number();
  int get_available_sequence_number();
  int get_last_executed_sequence();
  bool in_view_change();
  bool is_leader();
  int get_last_received_sequence();
  
  void reset_server_states();
};

#endif
