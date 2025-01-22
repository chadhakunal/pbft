#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <iostream>
#include <string>

void save_private_key_to_der(const CryptoPP::RSA::PrivateKey& privateKey, const std::string& private_key_path) {
    CryptoPP::FileSink file(private_key_path.c_str());
    privateKey.DEREncode(file);
    file.MessageEnd();
}

void save_public_key_to_der(const CryptoPP::RSA::PublicKey& publicKey, const std::string& public_key_path) {
    CryptoPP::FileSink file(public_key_path.c_str());
    publicKey.DEREncode(file);
    file.MessageEnd();
}

void generate_and_save_rsa_key_pair(const std::string& private_key_path, const std::string& public_key_path) {
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSA::PrivateKey privateKey;
    privateKey.GenerateRandomWithKeySize(rng, 2048);

    CryptoPP::RSA::PublicKey publicKey(privateKey);

    // Save keys to DER files
    save_private_key_to_der(privateKey, public_key_path);
    save_public_key_to_der(publicKey, private_key_path);

    std::cout << "RSA key pair generated and saved to DER files: " 
              << private_key_path << " and " << public_key_path << std::endl;
}

int main() {
    try {
        // Generate server keys
        for (int i = 0; i < 7; i++) {
            std::string private_key_path = "keys/private_key_" + std::to_string(i) + ".der";
            std::string public_key_path = "keys/public_key_" + std::to_string(i) + ".der";
            generate_and_save_rsa_key_pair(private_key_path, public_key_path);
        }

        // Generate client keys
        for (int i = 0; i < 10; i++) {
            std::string private_key_path = "keys/private_key_client_" + std::to_string(i) + ".der";
            std::string public_key_path = "keys/public_key_client_" + std::to_string(i) + ".der";
            generate_and_save_rsa_key_pair(private_key_path, public_key_path);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// #include <cryptopp/rsa.h>
// #include <cryptopp/osrng.h>
// #include <cryptopp/base64.h>
// #include <cryptopp/files.h>
// #include <iostream>
// #include <fstream>
// #include <stdexcept>
// #include <string>

// void save_private_key_to_pem(const CryptoPP::RSA::PrivateKey& privateKey, const std::string& private_key_path) {
//     std::ofstream file(private_key_path);
//     if (!file.is_open()) {
//         throw std::runtime_error("Failed to open file for writing private key.");
//     }

//     file << "-----BEGIN RSA PRIVATE KEY-----\n";
//     CryptoPP::Base64Encoder encoder(new CryptoPP::FileSink(file));
//     privateKey.DEREncode(encoder);
//     encoder.MessageEnd();
//     file << "-----END RSA PRIVATE KEY-----\n";
//     file.close();
// }

// void save_public_key_to_pem(const CryptoPP::RSA::PublicKey& publicKey, const std::string& public_key_path) {
//     std::ofstream file(public_key_path);
//     if (!file.is_open()) {
//         throw std::runtime_error("Failed to open file for writing public key.");
//     }

//     file << "-----BEGIN PUBLIC KEY-----\n";
//     CryptoPP::Base64Encoder encoder(new CryptoPP::FileSink(file));
//     publicKey.DEREncode(encoder);
//     encoder.MessageEnd();
//     file << "-----END PUBLIC KEY-----\n";
//     file.close();
// }

// void generate_and_save_rsa_key_pair(const std::string& private_key_path, const std::string& public_key_path) {
//     CryptoPP::AutoSeededRandomPool rng;
//     CryptoPP::RSA::PrivateKey privateKey;
//     privateKey.GenerateRandomWithKeySize(rng, 2048);

//     CryptoPP::RSA::PublicKey publicKey(privateKey);

//     // Save keys to PEM files
//     save_private_key_to_pem(privateKey, private_key_path);
//     save_public_key_to_pem(publicKey, public_key_path);

//     std::cout << "RSA key pair generated and saved to files: " 
//               << private_key_path << " and " << public_key_path << std::endl;
// }

// int main() {
//     try {
//         // Generate server keys
//         for (int i = 0; i < 7; i++) {
//             std::string private_key_path = "keys/private_key_" + std::to_string(i) + ".key";
//             std::string public_key_path = "keys/public_key_" + std::to_string(i) + ".pub";
//             generate_and_save_rsa_key_pair(private_key_path, public_key_path);
//         }

//         // Generate client keys
//         for (int i = 0; i < 10; i++) {
//             std::string private_key_path = "keys/private_key_client_" + std::to_string(i) + ".key";
//             std::string public_key_path = "keys/public_key_client_" + std::to_string(i) + ".pub";
//             generate_and_save_rsa_key_pair(private_key_path, public_key_path);
//         }
//     } catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }

// #include <openssl/rsa.h>
// #include <openssl/pem.h>
// #include <openssl/err.h>
// #include <iostream>
// #include <memory>
// #include <stdexcept>
// #include <string>
// #include <filesystem>

// void generate_and_save_rsa_key_pair(const std::string& private_key_path, const std::string& public_key_path) {
//     int key_length = 2048;
//     unsigned long exponent = RSA_F4;
//     RSA* rsa = RSA_new();

//     BIGNUM* bn = BN_new();
//     if (!BN_set_word(bn, exponent) || !RSA_generate_key_ex(rsa, key_length, bn, nullptr)) {
//         BN_free(bn);
//         RSA_free(rsa);
//         throw std::runtime_error("Failed to generate RSA key pair.");
//     }
//     BN_free(bn);

//     FILE* private_key_file = fopen(private_key_path.c_str(), "wb");
//     if (!private_key_file || !PEM_write_RSAPrivateKey(private_key_file, rsa, nullptr, nullptr, 0, nullptr, nullptr)) {
//         fclose(private_key_file);
//         RSA_free(rsa);
//         throw std::runtime_error("Failed to write private key to file.");
//     }
//     fclose(private_key_file);

//     FILE* public_key_file = fopen(public_key_path.c_str(), "wb");
//     if (!public_key_file || !PEM_write_RSA_PUBKEY(public_key_file, rsa)) {
//         fclose(public_key_file);
//         RSA_free(rsa);
//         throw std::runtime_error("Failed to write public key to file.");
//     }
//     fclose(public_key_file);

//     RSA_free(rsa);
//     std::cout << "RSA key pair generated and saved to files successfully.\n";
// }

// int main() {
//     try {
//         for(int i = 0; i < 7; i++) {
//             std::string private_key_path = "keys/private_key_" + std::to_string(i) + ".key";
//             std::string public_key_path = "keys/public_key_" + std::to_string(i) + ".pub";
//             generate_and_save_rsa_key_pair(private_key_path, public_key_path);
//         }

//         for(int i = 0; i < 10; i++) {
//             std::string private_key_path = "keys/private_key_client_" + std::to_string(i) + ".key";
//             std::string public_key_path = "keys/public_key_client_" + std::to_string(i) + ".pub";
//             generate_and_save_rsa_key_pair(private_key_path, public_key_path);
//         }
//     } catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }