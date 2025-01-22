#include "utils.h"

template void set_signature<CommitRequest>(CommitRequest&);
template void set_signature<PrepareRequest>(PrepareRequest&);
template void set_signature<PrePrepareRequest>(PrePrepareRequest&);
template void set_signature<ViewChangeRequest>(ViewChangeRequest&);
template void set_signature<TransactionReplyRequest>(TransactionReplyRequest&);
template bool verify_signature<CommitRequest>(CommitRequest&);
template bool verify_signature<PrepareRequest>(PrepareRequest&);
template bool verify_signature<PrePrepareRequest>(PrePrepareRequest&);
template bool verify_signature<ViewChangeRequest>(ViewChangeRequest&);
template bool verify_signature<CommittedRequest>(CommittedRequest&);
template bool verify_signature<const CommitRequest>(const CommitRequest&);
template bool verify_signature<const PrepareRequest>(const PrepareRequest&);
template bool verify_signature<const PrePrepareRequest>(const PrePrepareRequest&);
template bool verify_signature<const CommittedRequest>(const CommittedRequest&);

std::string to_hex_string(const unsigned char* hash, size_t length) {
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string hash_transaction(const TransactionRequestEntry transaction) {
    std::string serialized_data;
    if (!transaction.SerializeToString(&serialized_data)) {
        throw std::runtime_error("Failed to serialize the transaction.");
    }

    std::vector<unsigned char> data(serialized_data.begin(), serialized_data.end());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);

    return to_hex_string(hash, SHA256_DIGEST_LENGTH);
}

template <typename T>
std::string serialize_proto_to_bytes(T& proto_message) {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be a subclass of google::protobuf::Message");

    google::protobuf::Message* protoCopy = proto_message.New();
    protoCopy->CopyFrom(proto_message);

    if (protoCopy->GetDescriptor()->FindFieldByName("signature")) {
        protoCopy->GetReflection()->SetString(protoCopy, 
                                              protoCopy->GetDescriptor()->FindFieldByName("signature"), 
                                              "");
    }
    
    std::string output;
    if (protoCopy->SerializeToString(&output)) {
        delete protoCopy;
        return output;
    } else {
        delete protoCopy;
        throw std::runtime_error("Failed to serialize protobuf message.");
    }
}

std::string encrypt_byte_string(const std::string& serialized_string_bytes) {
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(private_key);

    std::string signature;

    CryptoPP::StringSource ss1(serialized_string_bytes, true,
        new CryptoPP::PK_EncryptorFilter(rng, encryptor,
            new CryptoPP::StringSink(signature)
        )
    );
    
    return signature;
}

std::string decrypt_signature(const std::string& signature, int public_key_id) {
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(public_keys[public_key_id]);
    std::string decrypted_data;
    CryptoPP::StringSource ss3(signature, true,
        new CryptoPP::PK_DecryptorFilter(rng, decryptor,
            new CryptoPP::StringSink(decrypted_data)
        )
    );

    return decrypted_data;
}

template <typename T>
void set_signature(T& proto_message) {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be a subclass of google::protobuf::Message");
                  
    std::string bytes = serialize_proto_to_bytes(proto_message);
    std::string signature = encrypt_byte_string(bytes);

    if (proto_message.GetDescriptor()->FindFieldByName("signature")) {
        proto_message.GetReflection()->SetString(&proto_message, 
                                                 proto_message.GetDescriptor()->FindFieldByName("signature"), 
                                                 signature);
    }
}

template <typename T>
bool verify_signature(T& proto_message) {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be a subclass of google::protobuf::Message");

    const google::protobuf::FieldDescriptor* signature_field = proto_message.GetDescriptor()->FindFieldByName("signature");
    const google::protobuf::FieldDescriptor* server_id_field = proto_message.GetDescriptor()->FindFieldByName("server_id");
    
    if (!signature_field) {
        throw std::runtime_error("Signature field not found in message.");
    }

    std::string signature = proto_message.GetReflection()->GetString(proto_message, signature_field);
    int signature_host_id = proto_message.GetReflection()->GetInt32(proto_message, server_id_field);
    if(signature_host_id == server_id) return true;

    std::string bytes = serialize_proto_to_bytes(proto_message);

    std::string decrypted_bytes = decrypt_signature(signature, signature_host_id);
    return decrypted_bytes == bytes;
}
