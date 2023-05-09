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

std::string RELAY_IP = "";
std::string WORKER_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int GROUP_SIZE;

void forwardVoice(const std::vector<char>& fileData) {
    rpc::client client(WORKER_IP, 8080);
    // Call the remote procedure to send the file
    client.call("sendVoice", voiceData);
}

void sendVoice(const std::vector<char>& fileData) {
    // Open a new file for writing
    FILE* file = fopen("received_voice", "wb");
    if (!file) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    // Write the file data
    if (fwrite(fileData.data(), 1, fileData.size(), file) != fileData.size()) {
        std::cerr << "Failed to write file" << std::endl;
        fclose(file);
        return;
    }

    // Close the file
    fclose(file);

    std::cout << "Voice packet received" << std::endl;
    forwardVoice(fileData);
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

    // Bind the sendVoice function to a remote procedure
    server.bind("sendVoice", sendVoice);

    // Run the server
    //server.async_run(1);
    server.run();
    // Close log file
    logFile.close();
    return 0;
}