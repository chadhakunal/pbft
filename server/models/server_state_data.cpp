#include "models/server_state_data.h"

int ServerStateData::current_view_number = 0;
int ServerStateData::available_sequence_number = 1;
int ServerStateData::last_executed_sequence = 0;
int ServerStateData::last_received_sequence = 0;
bool ServerStateData::view_change = false;

ServerStateData::ServerStateData() = default;

void ServerStateData::reset_server_states() {
    std::unique_lock<std::shared_mutex> lock(mtx);
    current_view_number = 0;
    available_sequence_number = 1;
    last_executed_sequence = 0;
    last_received_sequence = 0;
    view_change = false;
}

int ServerStateData::update_state(const std::unordered_map<std::string, std::string>& new_state) {
    std::unique_lock<std::shared_mutex> lock(mtx);

    static const std::unordered_map<std::string, std::function<void(const std::string&)>> setters = {
        {"current_view_number", [](const std::string& val) { current_view_number = std::stoi(val); }},
        {"available_sequence_number", [](const std::string& val) { available_sequence_number = std::stoi(val); }},
        {"last_executed_sequence", [](const std::string& val) { last_executed_sequence = std::stoi(val); }},
        {"last_received_sequence", [](const std::string& val) { last_received_sequence = std::stoi(val); }},
        {"view_change", [](const std::string& val) { view_change = (val == "true"); }},
    };

    for (const auto& [key, value] : new_state) {
        auto it = setters.find(key);
        if (it != setters.end()) {
            it->second(value);
        } else {
            std::cerr << "Error: Invalid attribute key \"" << key << "\"\n";
            return -1;
        }
    }

    return 0;
}

int ServerStateData::get_current_view_number() {
    std::shared_lock<std::shared_mutex> lock(mtx); 
    return current_view_number;
}

int ServerStateData::get_available_sequence_number() {
    std::shared_lock<std::shared_mutex> lock(mtx); 
    return available_sequence_number;
}

int ServerStateData::get_last_executed_sequence() {
    std::shared_lock<std::shared_mutex> lock(mtx); 
    return last_executed_sequence;
}

int ServerStateData::get_last_received_sequence() {
    std::shared_lock<std::shared_mutex> lock(mtx); 
    return last_received_sequence;
}

bool ServerStateData::in_view_change() {
    std::shared_lock<std::shared_mutex> lock(mtx); 
    return view_change;
}

bool ServerStateData::is_leader() {
    std::shared_lock<std::shared_mutex> lock(mtx);
    return (current_view_number % SERVER_HOSTS.size()) == server_id;
}