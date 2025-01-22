#include "client.h"

std::vector<std::unique_ptr<grpc::Server> > client_servers(CLIENT_HOSTS.size());
std::atomic<bool> shutdown_flag{false};

int prompt(std::vector<std::unique_ptr<PbftServerInterface> >& servers) {
    int choice;
    while(true) {
        std::cout<<std::endl;
        std::cout<<"Select an option:"<<std::endl;
        std::cout<<"1. Print Log"<<std::endl;
        std::cout<<"2. Print DB"<<std::endl;
        std::cout<<"3. Print Status"<<std::endl;
        std::cout<<"4. Print View"<<std::endl;
        std::cout<<"5. Performance"<<std::endl;
        std::cout<<"6. Send set transactions to servers"<<std::endl;
        std::cout<<"7. Exit"<<std::endl;

        std::cout<<"Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1: {
                int server_id;
                std::cout<<"Enter Server Id: ";
                std::cin >> server_id;

                std::cout << "Server ID " << server_id << ":\n";
                PrintLogResponse response = servers[server_id - 1]->get_logs();

                for (int i = 0; i < response.checkpoint_entries_size(); ++i) {
                    const CheckpointLogEntry& checkpoint_entry = response.checkpoint_entries(i);
                    std::cout << "\tCheckpoint Entry " << i + 1 << ":\n";
                    std::cout << "\t\tView Number: " << checkpoint_entry.view_number() << "\n";
                    std::cout << "\t\tSequence Number: " << checkpoint_entry.sequence_number() << "\n";
                    std::cout << "\t\tCheckpointRequests:\n";
                    for (int j = 0; j < checkpoint_entry.checkpoint_request_size(); ++j) {
                        const CheckpointRequest& checkpoint_request = checkpoint_entry.checkpoint_request(j);
                        std::cout << "\t\t\tCheckpoint " << j + 1 << ":\n";
                        std::cout << "\t\t\t\tView Number: " << checkpoint_request.view_number() << "\n";
                        std::cout << "\t\t\t\tSequence Number: " << checkpoint_request.sequence_number() << "\n";
                        std::cout << "\t\t\t\tState Digest: " << checkpoint_request.state_digest() << "\n";
                        std::cout << "\t\t\t\tServer ID: " << checkpoint_request.server_id() + 1 << "\n";
                    }
                }

                for (int i = 0; i < response.log_entries_size(); ++i) {
                    const LogEntry& log_entry = response.log_entries(i);

                    // Print PrePrepareRequest details
                    const PrePrepareRequest& preprepare = log_entry.preprepare();
                    std::cout << "\tLog Entry " << i + 1 << ":\n";
                    std::cout << "\t\tPrePrepareRequest:\n";
                    std::cout << "\t\t\tView Number: " << preprepare.view_number() << "\n";
                    std::cout << "\t\t\tSequence Number: " << preprepare.sequence_number() << "\n";
                    std::cout << "\t\t\tMessage Digest: " << preprepare.message_digest() << "\n";
                    std::cout << "\t\t\tServer ID: " << preprepare.server_id() + 1 << "\n";

                    // Print PrepareRequests
                    std::cout << "\t\tPrepareRequests:\n";
                    for (int j = 0; j < log_entry.prepares_size(); ++j) {
                        const PrepareRequest& prepare = log_entry.prepares(j);
                        std::cout << "\t\t\tPrepare " << j + 1 << ":\n";
                        std::cout << "\t\t\t\tView Number: " << prepare.view_number() << "\n";
                        std::cout << "\t\t\t\tSequence Number: " << prepare.sequence_number() << "\n";
                        std::cout << "\t\t\t\tMessage Digest: " << prepare.message_digest() << "\n";
                        std::cout << "\t\t\t\tServer ID: " << prepare.server_id() + 1 << "\n";
                    }

                    // Print CommitRequests
                    std::cout << "\t\tCommitRequests:\n";
                    for (int k = 0; k < log_entry.commits_size(); ++k) {
                        const CommitRequest& commit = log_entry.commits(k);
                        std::cout << "\t\t\tCommit " << k + 1 << ":\n";
                        std::cout << "\t\t\t\tView Number: " << commit.view_number() << "\n";
                        std::cout << "\t\t\t\tSequence Number: " << commit.sequence_number() << "\n";
                        std::cout << "\t\t\t\tMessage Digest: " << commit.message_digest() << "\n";
                        std::cout << "\t\t\t\tServer ID: " << commit.server_id() + 1 << "\n";
                    }

                    // Print TransactionReplyRequest details if present
                    if (log_entry.has_reply()) {
                        const TransactionReplyRequest& reply = log_entry.reply();
                        std::cout << "\t\tTransactionReplyRequest:\n";
                        std::cout << "\t\t\tView Number: " << reply.view_number() << "\n";
                        std::cout << "\t\t\tTimestamp: " << reply.timestamp() << "\n";
                        std::cout << "\t\t\tClient ID: " << reply.client_id() << "\n";
                        std::cout << "\t\t\tServer ID: " << reply.server_id() + 1 << "\n";
                        std::cout << "\t\t\tBalances Digest: " << reply.balances_digest() << "\n";
                    }
                    std::cout << "\n";
                }
                std::cout << "--------------------------\n";
                break;
            }
            case 2:
                std::cout << std::setw(4) << "";

                for (int client_id = 0; client_id < 10; client_id++) {
                    std::cout << std::setw(8) << static_cast<char>('A' + client_id);
                }
                std::cout << "\n";
                for (int k = 0; k < servers.size(); ++k) {
                    std::cout << std::setw(4) << servers[k]->get_server_id_string();

                    PrintDBResponse response = servers[k]->get_db();
                    std::map<int, int> balance_map;

                    for (int i = 0; i < response.balance_size(); ++i) {
                        balance_map[response.balance(i).client_id()] = response.balance(i).amount();
                    }

                    for (int client_id = 0; client_id < 10; client_id++) {
                        if (balance_map.find(client_id) != balance_map.end()) {
                            std::cout << std::setw(8) << balance_map[client_id];
                        } else {
                            std::cout << std::setw(8) << "-";
                        }
                    }
                    std::cout << "\n";
                }
                break;
            case 3: {
                int seq_num;
                std::cout << "Enter Sequence Number: ";
                std::cin >> seq_num;
                std::cout << std::left << std::setw(12) << "Server ID";
                for (int k = 0; k < servers.size(); k++) {
                    std::cout << std::left << std::setw(12) << servers[k]->get_server_id_string();
                }
                std::cout << std::endl;

                std::cout << std::left << std::setw(12) << "Status";
                for (int k = 0; k < servers.size(); k++) {
                    std::cout << std::left << std::setw(12) << servers[k]->get_status(seq_num);
                }
                std::cout << std::endl;
                break;
            }
            case 4: {
                int server_id;
                std::cout<<"Enter Server Id: ";
                std::cin >> server_id;

                std::cout << "Server ID " << server_id << ":\n";
                PrintViewResponse response = servers[server_id - 1]->get_new_view_requests();

                std::sort(response.mutable_new_view_request()->begin(),
                    response.mutable_new_view_request()->end(),
                    [](const auto& lhs, const auto& rhs) {
                        return lhs.view_number() < rhs.view_number();
                    });

                for (const auto& new_view_request : response.new_view_request()) {
                    std::cout << "NewViewRequest:" << std::endl;
                    std::cout << "  Server ID: " << new_view_request.server_id() + 1 << std::endl;
                    std::cout << "  View Number: " << new_view_request.view_number() << std::endl;

                    std::cout << "  ViewChangeRequests:" << std::endl;
                    for (const auto& view_change_request : new_view_request.view_change_requests()) {
                        std::cout << "    Checkpoint Sequence Number: " << view_change_request.checkpoint_sequence_number() << std::endl;
                        std::cout << "    Server ID: " << view_change_request.server_id() + 1 << std::endl;
                    }

                    std::cout << "  PrePrepareRequests:" << std::endl;
                    for (const auto& preprepare_request : new_view_request.preprepare_requests()) {
                        std::cout << "    View Number: " << preprepare_request.view_number() << std::endl;
                        std::cout << "    Sequence Number: " << preprepare_request.sequence_number() << std::endl;
                        std::cout << "    Message Digest: " << preprepare_request.message_digest() << std::endl;
                        std::cout << "    Server ID: " << preprepare_request.server_id() + 1 << std::endl;
                    }
                }
                break;
            }
            case 5: {
                double total_latency = 0;
                double total_throughput = 0;
                int column_width = 8;

                std::cout<<"--------------------------------------------------------------------------------------------"<<std::endl;
                std::cout<<std::setw(12)<<" "<<"\t";  
                for (int i = 0; i < servers.size(); i++) {
                    std::cout<<std::setw(column_width)<<servers[i]->get_server_id_string()<<"\t";
                }
                std::cout<<std::endl;
                std::cout<<"--------------------------------------------------------------------------------------------"<<std::endl;

                std::vector<std::vector<double>> performance_metrics;
                for (int i = 0; i < servers.size(); i++) {
                    std::vector<double> metrics = servers[i]->get_performance();
                    total_throughput += metrics[0];
                    total_latency += metrics[1];
                    performance_metrics.push_back(metrics);
                }

                std::cout<<std::setw(12)<<"Throughput(t/s)"<<"\t";
                for (int i = 0; i < servers.size(); i++) {
                    std::cout<<std::setw(column_width)<<std::fixed<<std::setprecision(2)<<performance_metrics[i][0]<<"\t";
                }
                std::cout<<std::endl;

                std::cout<<std::setw(12)<<"Latency(ms)"<<"\t";
                for (int i = 0; i < servers.size(); i++) {
                    std::cout<<std::setw(column_width)<<std::fixed<<std::setprecision(2)<<performance_metrics[i][1]<<"\t";
                }
                std::cout<<std::endl;
                std::cout<<"--------------------------------------------------------------------------------------------"<<std::endl;

                std::cout<<"Average Throughput: "<<std::fixed<<std::setprecision(2)<<total_throughput / servers.size()<<" transactions/sec"<<std::endl;
                std::cout<<"Average Latency: "<<std::fixed<<std::setprecision(2)<<(1000 * total_latency) / servers.size()<<"ms"<<std::endl;
                break;
            }
            case 6:
                return 1;
            case 7:
                std::cout<<"Exiting..."<<std::endl;
                return 0;
            default:
                std::cout<<"Invalid choice. Please select again."<<std::endl;
                break;
        }
    }
}

std::vector<std::unique_ptr<TransactionSet>> build_transaction_sets() {
    std::ifstream tests;
    std::string line, temp;
    std::vector<std::unique_ptr<TransactionSet> > sets;
    int sets_size = 0;
    std::string current_set = "";

    tests.open(TEST_CASE_PATH);

    if (!tests.is_open()) {
        std::cerr << "Error opening file at " << TEST_CASE_PATH << std::endl;
        return sets;
    }

    while (std::getline(tests, line)) {
        std::stringstream ss(line);
        std::string set_number_str, transaction, live_servers_str, byzantine_servers_str;
        std::vector<std::string> live_servers_list;
        std::vector<std::string> byzantine_servers_list;

        std::getline(ss, set_number_str, ',');
        std::getline(ss, transaction, ')');
        std::getline(ss, live_servers_str, ']');
        std::getline(ss, byzantine_servers_str, ']');

        boost::trim(set_number_str);
        if(set_number_str != "" && set_number_str != current_set) {
            current_set = set_number_str;

            live_servers_str = live_servers_str.length() > 3 ? live_servers_str.substr(4, live_servers_str.size() - 3) : "";
            if(live_servers_str.length() > 0) {
                boost::split(live_servers_list, live_servers_str, boost::is_any_of(","), boost::token_compress_on);
                for (auto& ls : live_servers_list) {
                    boost::trim(ls);
                    if (!ls.empty() && ls[0] == 'S') {
                        ls = ls.substr(1);
                    }
                }
            }

            byzantine_servers_str = byzantine_servers_str.length() > 3 ? byzantine_servers_str.substr(4, byzantine_servers_str.size() - 1) : "";
            boost::trim(byzantine_servers_str);

            // Check for and remove quotes if they are present
            if (!byzantine_servers_str.empty() && byzantine_servers_str.front() == '"' && byzantine_servers_str.back() == '"') {
                byzantine_servers_str = byzantine_servers_str.substr(1, byzantine_servers_str.size() - 2);
            }

            if (byzantine_servers_str.length() > 0) {
                boost::split(byzantine_servers_list, byzantine_servers_str, boost::is_any_of(","), boost::token_compress_on);
                for (auto& bs : byzantine_servers_list) {
                    boost::trim(bs);
                    if (!bs.empty() && bs[0] == 'S') {
                        bs = bs.substr(1);
                    }
                }
            }

            sets.push_back(std::make_unique<TransactionSet>(set_number_str, live_servers_list, byzantine_servers_list));
            sets_size++;
        }

        transaction = transaction.substr(2, transaction.size()-2);
        sets[sets_size-1]->addTransaction(transaction);
    }

    return sets;
}

void handleSignal(int signum) {
    shutdown_flag = true;
    for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
        if(client_servers[i]) {
            std::cout<<"Terminating Client Server "<<i+1<<"!"<<std::endl;
            client_servers[i]->Shutdown();
        }
    }
}

int main() {
    std::vector<std::unique_ptr<TransactionSet> > sets = build_transaction_sets();
    std::vector<std::unique_ptr<PbftServerInterface> > servers(SERVER_HOSTS.size());
    std::vector<std::unique_ptr<PbftClient> > clients(CLIENT_HOSTS.size());
    std::vector<std::thread> client_req_threads;
    
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    for(int i = 0; i < SERVER_HOSTS.size(); i++) {
        servers[i] = std::make_unique<PbftServerInterface>(i, grpc::CreateChannel(SERVER_HOSTS[i], grpc::InsecureChannelCredentials()));
    }
    
    for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
        clients[i] = std::make_unique<PbftClient>(i, servers);
    }

    for (int i = 0; i < CLIENT_HOSTS.size(); i++) {
        client_req_threads.push_back(std::thread([&clients, i]() {
            ServerBuilder builder;
            builder.AddListeningPort(CLIENT_HOSTS[i], grpc::InsecureServerCredentials());
            int max_threads = 10;
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS, max_threads);
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, max_threads);
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, max_threads);
            
            builder.RegisterService(clients[i].get());

            client_servers[i] = builder.BuildAndStart();
            client_servers[i]->Wait();
        }));
    }

    int prompt_res;
    prompt_res = prompt(servers);
    if(!prompt_res) return 0;

    for(int i = 0; !shutdown_flag && i < sets.size(); i++) {
        std::vector<bool> live_servers = sets[i]->get_live_servers();
        std::vector<bool> byzantine_servers = sets[i]->get_byzantine_servers();
        std::vector<std::vector<int>> transactions = sets[i]->get_transactions();
        std::string id = sets[i]->get_set_id();

        for(int k = 0; k < servers.size(); k++) {
            servers[k]->reset_server();
        }

        for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
            clients[i]->reset();
        }

        std::cout << "Set: " << id << std::endl;
        std::cout << "Live Server States:" << std::endl;
        std::cout << std::left << std::setw(12) << "Server ID";
        for (int k = 0; k < servers.size(); k++) {
            std::cout << std::left << std::setw(8) << ("S" + std::to_string(k + 1));
        }
        std::cout << std::endl;
        std::cout << std::left << std::setw(12) << "Status";
        for (int k = 0; k < servers.size(); k++) {
            std::cout << std::left << std::setw(8) << (live_servers[k] ? "Up" : "Down");
            if (live_servers[k]) {
                servers[k]->server_up();
            } else {
                servers[k]->server_down();
            }
        }
        std::cout << std::endl << std::endl;
        std::cout << "Byzantine Server States:" << std::endl;
        std::cout << std::left << std::setw(12) << "Server ID";
        for (int k = 0; k < servers.size(); k++) {
            std::cout << std::left << std::setw(8) << ("S" + std::to_string(k + 1));
        }
        std::cout << std::endl;
        std::cout << std::left << std::setw(12) << "Faulty";
        for (int k = 0; k < servers.size(); k++) {
            std::cout << std::left << std::setw(8) << (byzantine_servers[k] ? "Yes" : "No");
            if (byzantine_servers[k]) {
                servers[k]->make_faulty();
            } else {
                servers[k]->make_non_faulty();
            }
        }
        std::cout << std::endl;
        
        for(auto transaction : transactions) {
            clients[transaction[0]]->push_transaction(transaction);
        }

        prompt_res = prompt(servers);
        if(!prompt_res || shutdown_flag) {
            break;
        };
    }

    for(int i = 0; i < CLIENT_HOSTS.size(); i++) {
        if(client_servers[i]) {
            std::cout<<"Terminating Client Server "<<i+1<<"!"<<std::endl;
            client_servers[i]->Shutdown();
        }
    }

    for (int i = 0; i < clients.size(); i++) {
        if (client_req_threads[i].joinable()) {
            client_req_threads[i].join();
        }
    }

    std::cout<<"Done!"<<std::endl;
    return 0;
}
