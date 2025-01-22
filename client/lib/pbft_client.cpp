#include "lib/pbft_client.h"

PbftClient::PbftClient(int id, std::vector<std::unique_ptr<PbftServerInterface>>& servers) : client_id(id), leader_id(0), servers(servers) {
    processing_thread = std::thread(&PbftClient::process_transactions, this);
};

PbftClient::~PbftClient() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_processing = true;
        queue_condition.notify_all();
    }
    processing_thread.join();
}

void PbftClient::push_transaction(const std::vector<int> transaction) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    transaction_queue.push(transaction);
    queue_condition.notify_one();
}

void PbftClient::process_transactions() {   
    while (true) {
        std::vector<int> transaction;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [this]() { return !transaction_queue.empty() || stop_processing; });

            if (stop_processing) {
                break;
            }

            transaction = transaction_queue.front();
            transaction_queue.pop();
        }
        
        auto now = std::chrono::system_clock::now();
        current_transaction_ts = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // std::cout<<"Sending Transaction ("<<transaction[0]<<", "<<transaction[1]<<", "<<transaction[2]<<")"<<std::endl;
        std::unique_ptr<TransactionResponse> response = servers[leader_id]->send_transaction(current_transaction_ts, transaction, client_id);        
        bool transaction_successful = false;

        while(!stop_processing && !transaction_successful) {
            int reply_timeout = 1000;
            while (!stop_processing && !transaction_successful && reply_timeout > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                reply_timeout -= 10;

                std::unique_lock<std::mutex> lock(transaction_replies_mutex);
                std::unordered_map<std::string, int> digest_count;
                for (const auto& reply : transaction_replies) {
                    digest_count[reply.second.balances_digest()]++;
                }

                for (const auto& entry : digest_count) {
                    if (entry.second >= reply_majority_count) {
                        transaction_successful = true;
                    }
                }
            }

            if(!stop_processing && !transaction_successful) {
                // std::cout << "Timeout reached. Retrying transaction request for (" << transaction[0] << ", " << transaction[1] << ", " << transaction[2] << ")" << std::endl;
                for (auto& server : servers) {
                    server->send_transaction(current_transaction_ts, transaction, client_id, true);
                }
            }
        }
        
        // std::cout<<"Done Waiting For Transaction ("<<transaction[0]<<", "<<transaction[1]<<", "<<transaction[2]<<")"<<std::endl;

        {
            std::unique_lock<std::mutex> lock(transaction_replies_mutex);
            transaction_replies.clear();
        }
    }
}

Status PbftClient::TransactionReply(ServerContext* context, const TransactionReplyRequest* req, google::protobuf::Empty* res) {
    // std::cout<<"Received reply! "<<req->timestamp()<<" "<<current_transaction_ts<<std::endl;
    if(req->timestamp() == current_transaction_ts && verify_signature(req->signature(), req->server_id())) {
        std::unique_lock<std::mutex> lock(transaction_replies_mutex);
        transaction_replies[req->server_id()] = *req;
    }

    return Status::OK;
}
