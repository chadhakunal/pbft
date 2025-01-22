#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

std::string to_hex_string(const unsigned char* hash, size_t length);
std::string hash_transaction(const TransactionRequestEntry transaction);
// mcl::bn256::G2 partial_sign(const mcl::bn256::Fr &private_key_share, std::string message_str);

template <typename T>
std::string serialize_proto_to_bytes(T& proto_message);
std::string encrypt_byte_string(const std::string& serialized_string_bytes);
std::string decrypt_signature(const std::string& signature);

template <typename T>
void set_signature(T& proto_message);

template <typename T>
bool verify_signature(T& proto_message);

#endif
