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

#define RAW_DB_SIZE 2288 * 64

std::string RELAY_IP = "";
std::string WORKER_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int GROUP_SIZE;
//char *raw_db;

/// 1. Receive snippet from client 
/// 2. Extend snippet to database
/// 3. Relay database to worker
void process(const std::vector<uint8_t>& snippet) {
    
    std::cout << "Hi from relay" << std::endl;
    std::vector<uint8_t> raw_db;
    for (int i = 0; i < RAW_DB_SIZE; i++) {
        raw_db.push_back(snippet[i % snippet.size()]);
    }

    rpc::client client(WORKER_IP, 8080);
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client.call("process", raw_db);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES relay ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "m:w:s:n:r:g:e:")) != -1) {
        switch(c) {
            case 'e':
                RELAY_IP = std::string(optarg);
                break;
            case 'w':
                WORKER_IP = std::string(optarg);
                break;
            case 'n':
                NUM_MESSAGE = std::stoi(optarg);
                break;
            case 'r':
                NUM_ROUNDS = std::stoi(optarg);
                break;
            case 'g':
                GROUP_SIZE = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }
    // Create a file for logging
    std::ofstream logFile("relay_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }
    // Create an RPC server
    rpc::server server(RELAY_IP, 8080);

    // Bind the relay function to a remote procedure
    server.bind("process", process);

    // Run the server
    //server.async_run(1);
    server.run();
    // Close log file
    logFile.close();
    return 0;
}
