#include "lib/server_interface.h"

PbftServerInterface::PbftServerInterface(int id, std::shared_ptr<Channel> channel) : server_id(id), stub_(PbftClientService::NewStub(channel)) {};

std::string PbftServerInterface::get_server_id_string() {
    std::string id = "S";
    id += std::to_string(server_id+1);
    return id;
}

void PbftServerInterface::server_up() {
    ClientContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    Status status = stub_->ServerUp(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `ServerUp` failed!"<<std::endl;
    }
}

void PbftServerInterface::server_down() {
    ClientContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    Status status = stub_->ServerDown(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `ServerDown` failed!"<<std::endl;
    }
}

void PbftServerInterface::make_faulty() {
    ClientContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    Status status = stub_->MakeFaulty(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `MakeFaulty` failed!"<<std::endl;
    }
}

void PbftServerInterface::make_non_faulty() {
    ClientContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    Status status = stub_->MakeNonFaulty(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `MakeNonFaulty` failed!"<<std::endl;
    }
}

void PbftServerInterface::reset_server() {
    ClientContext context;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    Status status = stub_->ResetServer(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `ResetServer` failed!"<<std::endl;
    }
}

std::unique_ptr<TransactionResponse> PbftServerInterface::send_transaction(std::int64_t timestamp, std::vector<int> transaction, int client_id, bool retrying) {
    ClientContext context;
    TransactionRequest request;
    request.set_retrying(retrying);
    TransactionRequestEntry* transaction_re = request.mutable_transaction();
    transaction_re->set_sender(transaction[0]);
    transaction_re->set_receiver(transaction[1]);
    transaction_re->set_amount(transaction[2]);
    transaction_re->set_client_id(client_id);
    transaction_re->set_timestamp(timestamp);

    auto response = std::make_unique<TransactionResponse>();
    Status status = stub_->SendTransaction(&context, request, response.get());

    if (!status.ok()) {
        std::cout<<"gRPC call `SendTransaction` failed! "<<server_id<<" "<<status.error_code()<<std::endl;
    }
    return response;
}

PrintLogResponse PbftServerInterface::get_logs() {
    ClientContext context;
    google::protobuf::Empty request;
    PrintLogResponse response;
    Status status = stub_->PrintLog(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `PrintLog` failed!"<<std::endl;
    }

    return response;
}

PrintViewResponse PbftServerInterface::get_new_view_requests() {
    ClientContext context;
    google::protobuf::Empty request;
    PrintViewResponse response;
    Status status = stub_->PrintView(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `PrintView` failed!"<<std::endl;
    }

    return response;
}

std::string PbftServerInterface::get_status(int sequence_number) {
    ClientContext context;
    PrintStatusRequest request;
    PrintStatusResponse response;

    request.set_sequence_number(sequence_number);

    Status status = stub_->PrintStatus(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `PrintStatus` failed!"<<std::endl;
    }
    
    if(response.status() == -1 || response.status() == 4) {
        return std::string("X");
    } else if(response.status() == 0) {
        return std::string("PP");
    } else if(response.status() == 1) {
        return std::string("P");
    } else if(response.status() == 2) {
        return std::string("C");
    } else if(response.status() == 3) {
        return std::string("E");
    }

    return std::string("X");
}

PrintDBResponse PbftServerInterface::get_db() {
    ClientContext context;
    google::protobuf::Empty request;
    PrintDBResponse response;
    Status status = stub_->PrintDB(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `PrintDB` failed!"<<std::endl;
    }

    return response;
}

std::vector<double> PbftServerInterface::get_performance() {
    ClientContext context;
    google::protobuf::Empty request;
    PerformanceResponse response;
    Status status = stub_->Performance(&context, request, &response);

    if (!status.ok()) {
        std::cout<<"gRPC call `Performance` failed!"<<std::endl;
    }

    std::vector<double> result = { response.throughput(), response.latency() };
    return result;
}
