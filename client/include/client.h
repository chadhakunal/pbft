#ifndef CLIENT_H
#define CLIENT_H

#include "globals.h"
#include "lib/pbft_client.h"
#include "lib/server_interface.h"
#include "lib/transaction_set.h"

int prompt(std::vector<std::unique_ptr<PbftClient> >& clients);

#endif
