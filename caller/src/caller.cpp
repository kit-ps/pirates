#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <pthread.h>
#include <openssl/aes.h>
#include "seal/seal.h"
#include "lpcnet/lpcnet_freedv.h"
#include <fstream>
#include <sys/wait.h>
#include <rpc/client.h>
#include <rpc/server.h>
#include <thread>

std::string CLIENT_IP = "";
std::string MASTER_IP = "";
std::string RELAY_IP = "";
int SNIPPET_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int GROUP_SIZE;

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES caller ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "c:m:s:n:r:e:")) != -1) {
        switch(c) {
            case 'c':
                CLIENT_IP = std::string(optarg);
                std::cout << "Caller IP is " << CLIENT_IP << std::endl;
                break;
            case 'e':
                RELAY_IP = std::string(optarg);
                break;
            case 's':
                SNIPPET_SIZE = std::stoi(optarg);
                break;
            case 'r':
                NUM_ROUNDS = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }
    // Create a file for logging
    std::ofstream logFile("caller_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open caller log!" << std::endl;
        return 1;
    }
    
    std::cout << "Encoding voice snippet ..." << std::endl;
    // Open input file
    FILE *input_file;
    std::string tmp = "/audio/raw_" + std::to_string(SNIPPET_SIZE) + "ms.wav";
    input_file = fopen(tmp.c_str(), "rb");
    if (!input_file) {
        std::cerr << "Failed to open raw audio file!" << std::endl;
        return 1;
    }

    std::vector<char> encoded_snippet;
    
    // direct_split = 0
    LPCNetFreeDV *lf = lpcnet_freedv_create(0);
    // Taken from lpcnet_enc.c
    char frame[lpcnet_bits_per_frame(lf)];
    short pcm[lpcnet_samples_per_frame(lf)];
    // Read from wav file and encode each read frame
    // Append frame to encoded_snippet vector
    auto start = std::chrono::high_resolution_clock::now();
    while (1) {      
        int nread = fread(pcm, sizeof(short), lpcnet_samples_per_frame(lf), input_file);
        if (nread != lpcnet_samples_per_frame(lf)) break;

        lpcnet_enc(lf, pcm, frame);
        for (char c : frame) {
            encoded_snippet.push_back(c);
        }
    }
    // Stop measuring time
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Needed time for LPCNet encoding: " << duration.count() << "\u03bcs\n";
     // Close files for encoding
    fclose(input_file);

    // Create a connection to the relay
    rpc::client client(RELAY_IP, 8080);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    // Call the remote procedure to send the file
    client.call("process", encoded_snippet);

    std::cout << "Caller goodbye ..." << std::endl;
    return 0;
}
