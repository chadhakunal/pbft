#ifndef MODELS_SERVER_LOGS
#define MODELS_SERVER_LOGS

#include "globals.h"

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
    }
};

struct PrePrepareRequestData {
    PrePrepareRequestState state;
    std::string message_digest;
    std::int64_t created_timestamp;
};

struct TransactionReplyRequestData {
    bool processed;
};

class LogStore {
 private:
    std::unordered_map<std::pair<int, int>, std::pair<PrePrepareRequest, PrePrepareRequestData>, PairHash> preprepare_log;
    std::unordered_map<std::pair<int, int>, std::unordered_map<int, PrepareRequest>, PairHash> prepare_log;
    std::unordered_map<std::pair<int, int>, std::unordered_map<int, CommitRequest>, PairHash> commit_log;
    std::unordered_map<std::pair<int, std::int64_t>, std::pair<TransactionReplyRequest, TransactionReplyRequestData>, PairHash> transaction_replies;

    std::unordered_map<std::pair<int, int>, std::unordered_map<int, CheckpointRequest>, PairHash> checkpoints;
    std::pair<int, int> last_stable_checkpoint;

    std::unordered_map<int, std::unordered_map<int, ViewChangeRequest> > view_change_logs;
    std::unordered_map<int, bool> view_change_majority;
    std::unordered_map<int, NewViewRequest> new_view_logs;

    std::mutex log_mutex;

    LogStore() {}

 public:
    LogStore(const LogStore&) = delete;
    LogStore& operator=(const LogStore&) = delete;

    static LogStore& get_instance();
    void add_preprepare_log(PrePrepareRequest request);
    void add_prepare_log(PrepareRequest request);
    void add_commit_log(CommitRequest request);
    bool preprepare_exists(int view_number, int sequence_number);
    PrePrepareRequestState get_preprepare_request_state(int view_number, int sequence_number);
    std::int64_t get_preprepare_request_creation_timestamp(int view_number, int sequence_number);
    void set_preprepare_request_state(int view_number, int sequence_number, PrePrepareRequestState request_state);
    bool is_digest_valid(int view_number, int sequence_number, std::string message_digest);
    int prepare_count(int view_number, int sequence_number);
    int commit_count(int view_number, int sequence_number);
    PrePrepareRequest preprepare_request(int view_number, int sequence_number);
    std::unordered_map<int, PrepareRequest> prepare_requests(int view_number, int sequence_number);
    std::unordered_map<int, CommitRequest> commit_requests(int view_number, int sequence_number);

    void add_checkpoint_request(CheckpointRequest req);
    std::unordered_map<int, CheckpointRequest> get_checkpoint_requests(std::pair<int, int> checkpoint_key);
    void add_transaction_reply(int client_id, std::int64_t timestamp);
    void transaction_reply_completed(int client_id, std::int64_t timestamp, TransactionReplyRequest transaction_reply);
    bool transaction_reply_exists(int client_id, std::int64_t timestamp);
    std::pair<TransactionReplyRequest, TransactionReplyRequestData> get_transaction_reply(int client_id, std::int64_t timestamp);
    std::pair<int, int> get_last_stable_checkpoint();

    std::unordered_map<int, ViewChangeRequest> get_view_change_requests(int view_number);
    void add_view_change_request(ViewChangeRequest request);
    int view_change_count(int view_number);
    bool my_view_change_exists(int view_number);

    bool is_view_change_majority_reached(int view_number);
    void view_change_majority_reached(int view_number);

    void add_new_view_request(NewViewRequest request);
    std::unordered_map<int, NewViewRequest> get_new_view_requests();
    bool new_view_request_exists(int view_number);


    PrintLogResponse get_print_log_response();

    void reset_log_store();
};


#endif