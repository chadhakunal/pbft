#ifndef PBFT_SERVER_H
#define PBFT_SERVER_H

#include "globals.h"
#include "models/client_data.h"
#include "lib/pbft_client_service_impl.h"
#include "lib/pbft_protocol_service_impl.h"

#include <signal.h>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;

#endif
