#include "models/log_store.h"

LogStore& LogStore::get_instance() {
    static LogStore instance;
    return instance;
}

void LogStore::reset_log_store() {
    std::lock_guard<std::mutex> lock(log_mutex);
    preprepare_log.clear();
    prepare_log.clear();
    commit_log.clear();
    transaction_replies.clear();
    checkpoints.clear();
    view_change_logs.clear();
    new_view_logs.clear();
    view_change_majority.clear();
    last_stable_checkpoint = {0, 0};
}

void LogStore::add_preprepare_log(PrePrepareRequest request) {
    PrePrepareRequestData preprepare_request_data;
    preprepare_request_data.state = PrePrepareRequestState::Init;
    preprepare_request_data.created_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(request.view_number(), request.sequence_number());
    preprepare_request_data.message_digest = request.message_digest();
    preprepare_log[key] = std::make_pair(request, preprepare_request_data);
}

void LogStore::add_prepare_log(PrepareRequest request) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(request.view_number(), request.sequence_number());
    prepare_log[key][request.server_id()] = request;
}

void LogStore::add_commit_log(CommitRequest request) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(request.view_number(), request.sequence_number());
    commit_log[key][request.server_id()] = request;
}

PrePrepareRequestState LogStore::get_preprepare_request_state(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return preprepare_log[key].second.state;
};

std::int64_t LogStore::get_preprepare_request_creation_timestamp(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return preprepare_log[key].second.created_timestamp;
}

void LogStore::set_preprepare_request_state(int view_number, int sequence_number, PrePrepareRequestState request_state) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    if(preprepare_log.find(key) != preprepare_log.end()) {
        preprepare_log[key].second.state = request_state;
    }
    return;
};

bool LogStore::preprepare_exists(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    if(preprepare_log.find(key) != preprepare_log.end()) {
        return true;
    }
    return false;
}

bool LogStore::is_digest_valid(int view_number, int sequence_number, std::string message_digest) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    if(preprepare_log.find(key) != preprepare_log.end() && preprepare_log[key].second.message_digest == message_digest) {
        return true;
    }
    return false;
}

int LogStore::prepare_count(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return preprepare_log.find(key) != preprepare_log.end() ? prepare_log[key].size() : 0;
}

int LogStore::commit_count(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return commit_log.find(key) != commit_log.end() ? commit_log[key].size() : 0;
}

PrePrepareRequest LogStore::preprepare_request(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return preprepare_log[key].first;
}

std::unordered_map<int, PrepareRequest> LogStore::prepare_requests(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return prepare_log[key];
}

std::unordered_map<int, CommitRequest> LogStore::commit_requests(int view_number, int sequence_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(view_number, sequence_number);
    return commit_log[key];
}

void LogStore::add_checkpoint_request(CheckpointRequest request) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(request.view_number(), request.sequence_number());
    checkpoints[key][request.server_id()] = request;
    // std::cout<<"ADDING CHECKPOINT: "<<request.view_number()<<", "<<request.sequence_number()<<", "<<request.server_id()<<std::endl;

    std::unordered_map<std::string, int> digest_count;
    
    for (const auto& [_, checkpoint_req] : checkpoints[key]) {
        digest_count[checkpoint_req.state_digest()]++;
    }

    for (const auto& [digest, count] : digest_count) {
        if (count >= checkpoint_majority_count) {
            for(int i = last_stable_checkpoint.second+1; i <= request.sequence_number(); i++) {
                auto pp_key = std::make_pair(request.view_number(), i);
                if(preprepare_log.find(pp_key) == preprepare_log.end() || preprepare_log[pp_key].second.state != PrePrepareRequestState::Executed) {
                     return;
                }
            }

            last_stable_checkpoint = std::make_pair(request.view_number(), request.sequence_number());
            // std::cout<<"MAJORITY FOR CHECKPOINT: "<<request.view_number()<<", "<<request.sequence_number()<<std::endl;
            
            for (auto it = checkpoints.begin(); it != checkpoints.end(); ) {
                if (it->first < last_stable_checkpoint) {
                    it = checkpoints.erase(it);
                } else {
                    it++;
                }
            }
            
            for (auto it = preprepare_log.begin(); it != preprepare_log.end(); ) {
                if (it->first <= last_stable_checkpoint) {
                    it = preprepare_log.erase(it);
                } else {
                    it++;
                }
            }

            for (auto it = prepare_log.begin(); it != prepare_log.end(); ) {
                if (it->first <= last_stable_checkpoint) {
                    it = prepare_log.erase(it);
                } else {
                    it++;
                }
            }

            for (auto it = commit_log.begin(); it != commit_log.end(); ) {
                if (it->first <= last_stable_checkpoint) {
                    it = commit_log.erase(it);
                } else {
                    it++;
                }
            }

            break;
        }
    }
}

std::unordered_map<int, CheckpointRequest> LogStore::get_checkpoint_requests(std::pair<int, int> checkpoint_key) {
    std::lock_guard<std::mutex> lock(log_mutex);
    return checkpoints[checkpoint_key];
}


std::pair<int, int> LogStore::get_last_stable_checkpoint() {
    return last_stable_checkpoint;
}

void LogStore::add_transaction_reply(int client_id, std::int64_t timestamp) {
    std::lock_guard<std::mutex> lock(log_mutex);
    TransactionReplyRequest transaction_reply;
    auto key = std::make_pair(client_id, timestamp);
    TransactionReplyRequestData additional_data;
    additional_data.processed = false;
    transaction_replies[key] = std::make_pair(transaction_reply, additional_data);
}

void LogStore::transaction_reply_completed(int client_id, std::int64_t timestamp, TransactionReplyRequest transaction_reply) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(client_id, timestamp);
    if(transaction_replies.find(key) != transaction_replies.end()) {
        transaction_replies[key].first = transaction_reply;
        transaction_replies[key].second.processed = true;
    } else {
        TransactionReplyRequestData additional_data;
        additional_data.processed = true;
        transaction_replies[key] = std::make_pair(transaction_reply, additional_data);
    }
    return;
}

bool LogStore::transaction_reply_exists(int client_id, std::int64_t timestamp) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(client_id, timestamp);
    if(transaction_replies.find(key) != transaction_replies.end()) {
        return true;
    }
    return false;
}

std::pair<TransactionReplyRequest, TransactionReplyRequestData> LogStore::get_transaction_reply(int client_id, std::int64_t timestamp) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto key = std::make_pair(client_id, timestamp);
    return transaction_replies[key];
}

std::unordered_map<int, ViewChangeRequest> LogStore::get_view_change_requests(int view_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    return view_change_logs[view_number];
}

int LogStore::view_change_count(int view_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    return view_change_logs[view_number].size();
}

bool LogStore::my_view_change_exists(int view_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if(view_change_logs.find(view_number) != view_change_logs.end()) {
        if(view_change_logs[view_number].find(server_id) != view_change_logs[view_number].end()) {
            return true;
        }
    }
    return false;
}

bool LogStore::is_view_change_majority_reached(int view_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if(view_change_majority.find(view_number) != view_change_majority.end()) {
       return view_change_majority[view_number];
    }
    return false;
}

void LogStore::view_change_majority_reached(int view_number) {
    std::lock_guard<std::mutex> lock(log_mutex);
    view_change_majority[view_number] = true;
}

void LogStore::add_view_change_request(ViewChangeRequest request) {
    std::lock_guard<std::mutex> lock(log_mutex);
    view_change_logs[request.next_view_number()][request.server_id()] = request;
}

void LogStore::add_new_view_request(NewViewRequest request) {
    std::lock_guard<std::mutex> lock(log_mutex);
    new_view_logs[request.view_number()] = request;
}

std::unordered_map<int, NewViewRequest> LogStore::get_new_view_requests() {
    std::lock_guard<std::mutex> lock(log_mutex);
    return new_view_logs;
}

bool LogStore::new_view_request_exists(int view_number) {
    if(new_view_logs.find(view_number) != new_view_logs.end()) {
        return true;
    }
    return false;   
}

PrintLogResponse LogStore::get_print_log_response() {
    PrintLogResponse response;
    
    for (const auto& entry : checkpoints) {
        const auto& key = entry.first;
        CheckpointLogEntry* checkpoint_entry = response.add_checkpoint_entries();
        checkpoint_entry->set_view_number(key.first);
        checkpoint_entry->set_sequence_number(key.second);

        for (const auto& checkpoint_requests : entry.second) {
            *checkpoint_entry->add_checkpoint_request() = checkpoint_requests.second;
        }
    }

    for (const auto& entry : preprepare_log) {
        const auto& key = entry.first;
        const auto& preprepare = entry.second.first;

        LogEntry* log_entry = response.add_log_entries();
        *log_entry->mutable_preprepare() = preprepare;

        if (prepare_log.find(key) != prepare_log.end()) {
            for (const auto& prepare_entry : prepare_log.at(key)) {
                *log_entry->add_prepares() = prepare_entry.second;
            }
        }

        if (commit_log.find(key) != commit_log.end()) {
            for (const auto& commit_entry : commit_log.at(key)) {
                *log_entry->add_commits() = commit_entry.second;
            }
        }

        std::pair<int, std::int64_t> reply_key = std::make_pair(preprepare.message().client_id(), static_cast<std::int64_t>(preprepare.message().timestamp()));
        if (transaction_replies.find(reply_key) != transaction_replies.end()) {
            *log_entry->mutable_reply() = transaction_replies.at(reply_key).first;
        }
    }

    return response;
}