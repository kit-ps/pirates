#include <cstdint>
#include <iostream>
#include <chrono>
#include <iterator>
#include <sys/types.h>
#include <vector>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <chrono>
#include <pthread.h>
#include "lpcnet/lpcnet_freedv.h"
#include <fstream>
#include <sys/wait.h>
#include <rpc/client.h>
#include <rpc/server.h>
#include <thread>
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/rng.h>
#include <botan/secmem.h>
#include "logging_helper.h"

std::string CLIENT_IP = "";
std::string MASTER_IP = "";
std::string RELAY_IP = "";
std::string RUN_ID = "";
int SNIPPET_SIZE;
int NUM_ROUNDS;
int GROUP_SIZE;
int NUM_USERS;

/// Read the snippet of size SNIPPET_SIZE from file and encode it with LPCNet
std::vector<uint8_t> encode_snippet() {
    // Open input file
    FILE *input_file;
    std::string tmp = "/audio/raw_" + std::to_string(SNIPPET_SIZE) + "ms.wav";
    input_file = fopen(tmp.c_str(), "rb");
    if (!input_file) {
        std::cerr << "Failed to open raw audio file!" << std::endl;
    }

    std::vector<uint8_t> encoded_snippet;
    
    // direct_split = 0
    LPCNetFreeDV *lf = lpcnet_freedv_create(0);
    // Taken from lpcnet_enc.c
    char frame[lpcnet_bits_per_frame(lf)];
    short pcm[lpcnet_samples_per_frame(lf)];
    // Read from wav file and encode each read frame
    // Append frame to encoded_snippet vector
    while (1) {      
        int nread = fread(pcm, sizeof(short), lpcnet_samples_per_frame(lf), input_file);
        if (nread != lpcnet_samples_per_frame(lf)) break;

        lpcnet_enc(lf, pcm, frame);
        for (char c : frame) {
            encoded_snippet.push_back((uint8_t)c);
        }
    }
    fclose(input_file);

    return encoded_snippet;
}

/// Encrypt the snippet using AES-CBC
std::vector<uint8_t> encrypt_snippet(std::vector<uint8_t> snippet) {
    const std::vector<uint8_t> key = Botan::hex_decode("C0FFEEC0FFEC0FFEEC0FFEC0FFEEC0FF");

    auto enc = Botan::Cipher_Mode::create("AES-128/CBC/PKCS7", Botan::ENCRYPTION);
    enc->set_key(key);

    // generate 0 nonce (IV)
    Botan::secure_vector<uint8_t> iv(0, enc->default_nonce_length());

    // Copy input data to a buffer that will be encrypted
    Botan::secure_vector<uint8_t> encrypted_snippet (
            snippet.begin(), 
            snippet.end());

    enc->start(iv);
    enc->finish(encrypted_snippet);

    return Botan::unlock(encrypted_snippet);
}

void process(int r) {

    uint64_t time_before = get_time(); 

    std::vector<uint8_t> encoded_snippet = encode_snippet();

    uint64_t time_after_encoding = get_time();

    std::cout << "I am showing you the start of the encoded snippet: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << ((int)encoded_snippet[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    std::vector<u_int8_t> encrypted_snippet = encrypt_snippet(encoded_snippet);

    uint64_t time_after_encryption = get_time();

    std::cout << "Original encrypted snippet length: " << encrypted_snippet.size() << std::endl;
    

    ///////////////////////// Send to Relay ////////////////////////////////////////////
    /*
    // Create a connection to the relay
    rpc::client *client = new rpc::client(RELAY_IP, 8080);
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client->call("process", encrypted_snippet);

    std::cout << "I am showing you the start of the encoded snippet: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << ((int)encoded_snippet[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    std::vector<u_int8_t> encrypted_snippet = encrypt_snippet(encoded_snippet);
    std::cout << "Original encrypted snippet length: " << encrypted_snippet.size() << std::endl;
    */

    ///////////////////////// Send to Relay ////////////////////////////////////////////
    // Create a connection to the relay
    rpc::client *client = new rpc::client(RELAY_IP, 8080);
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client->call("process", r, encrypted_snippet);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            delete client;
            client = new rpc::client(RELAY_IP, 8080);
        }
    }
    std::string log_content = RUN_ID + "-"
        + std::to_string(r) + ","
        + std::to_string(GROUP_SIZE) + "-" + std::to_string(NUM_USERS) + "-" + std::to_string(SNIPPET_SIZE) + ","
        + std::to_string(time_before) + ","
        + std::to_string(time_after_encoding) + ","
        + std::to_string(time_after_encryption);
        
    write_log("caller", log_content);
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES caller ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "o:r:s:g:u:x:n:")) != -1) {
        switch(c) {
            case 'o':
                CLIENT_IP = std::string(optarg);
                break;
            case 'r':
                NUM_ROUNDS = std::stoi(optarg);
                break;
            case 's':
                SNIPPET_SIZE = std::stoi(optarg);
                break;
            case 'g':
                GROUP_SIZE = std::stoi(optarg);
                break;
            case 'u':
                NUM_USERS = std::stoi(optarg);
                break;
            case 'x':
                RUN_ID = std::string(optarg);
                break;
            case 'n':
                RELAY_IP = std::string(optarg);
                break;
            default:
                abort();
        }
    }
    
    for (int i = 0; i < NUM_ROUNDS; i++) {
        process(i);
    }

    return 0;
}
