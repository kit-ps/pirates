#include <cstdint>
#include <iostream>
#include <array>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/aes.h>
#include "seal/seal.h"
#include <fstream>
#include<rpc/server.h>
#include<rpc/client.h>
#include"logging_helper.h"

#define RAW_DB_SIZE 1152 * 64

std::string RELAY_IP = "";
std::string WORKER_IP = "";
std::string RUN_ID = "";
int SNIPPET_SIZE;
int NUM_ROUNDS;
int GROUP_SIZE;
int NUM_USERS;
//char *raw_db;

/// 1. Receive snippet from client 
/// 2. Extend snippet to database
/// 3. Relay database to worker
void process(int r, const std::vector<uint8_t>& snippet) {
    
    std::cout << "Hi from relay" << std::endl;

    uint64_t time_before_relay = get_time();

    //std::cout << "Got a snippet of length " << snippet.size() << std::endl;
    std::vector<std::vector<uint8_t>> raw_db;
    int items_per_bucket = (int) 3 * NUM_USERS / (1.5 * (GROUP_SIZE-1));

    std::cout << "Items per bucket: " << items_per_bucket << std::endl;
    for (int i = 0; i < items_per_bucket; i++) {
        raw_db.push_back(snippet);
    }

    uint64_t time_after_relay = get_time();

    rpc::client *client = new rpc::client(WORKER_IP, 8080);
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client->call("process", r, raw_db);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            delete client;
            client = new rpc::client(WORKER_IP, 8080);
        }
    }

    std::string log_content = RUN_ID + "-"
        + std::to_string(r) + ","
        + std::to_string(time_before_relay) + ","
        + std::to_string(time_after_relay);
        
    write_log("relay", log_content);
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES relay ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "o:r:s:g:u:x:n:")) != -1) {
        switch(c) {
            case 'o':
                RELAY_IP = std::string(optarg);
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
                WORKER_IP = std::string(optarg);
                break;
            default:
                abort();
        }
    }
    // Create an RPC server
    rpc::server server(RELAY_IP, 8080);

    // Bind the relay function to a remote procedure
    server.bind("process", process);

    // Run the server
    //server.async_run(1);
    server.run();
    return 0;
}
