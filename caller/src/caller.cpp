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
    std::cout << "Starting PIRATES client ..." << std::endl;
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
    // Input and output files for input and encoded audio
    FILE *fin_enc, *fout_enc, *fin_dec, *fout_dec;
    std::string tmp = "/audio/raw_" + std::to_string(SNIPPET_SIZE) + "ms.wav";
    fin_enc = fopen(tmp.c_str(), "rb");
    if (fin_enc == NULL) {
        std::cerr << "Failed to open raw audio file!" << std::endl;
        return 1;
    }
    fout_enc = fopen("/home/app/pirates/raw_encoded", "wb");
    
    // direct_split = 0
    LPCNetFreeDV *lf = lpcnet_freedv_create(0);
    // Taken from lpcnet_enc.c
    char frame[lpcnet_bits_per_frame(lf)];
    int f=0;
    int bits_written=0;
    short pcm[lpcnet_samples_per_frame(lf)];
    // Read from wav file and encode each read frame
    // To test, we only encode 1 frame
    // How long is a LPCNet frame?
    auto start = std::chrono::high_resolution_clock::now();
    while (1) {      
        int nread = fread(pcm, sizeof(short), lpcnet_samples_per_frame(lf), fin_enc);
        if (nread != lpcnet_samples_per_frame(lf)) break;

        lpcnet_enc(lf, pcm, frame);
        bits_written += fwrite(frame, sizeof(char), lpcnet_bits_per_frame(lf), fout_enc);

        fflush(stdin);
        fflush(stdout);
        f++;
    }
    // Stop measuring time
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Needed time for LPCNet encoding: " << duration.count() << "\u03bcs\n";
     // Close files for encoding
    fclose(fin_enc);
    fclose(fout_enc);
    // Reopen encoded file
    FILE* encoded_snippet = fopen("/home/app/pirates/raw_encoded", "rb");
    if (!encoded_snippet) {
        std::cerr << "Failed to open encoded voice snippet" << std::endl;
        return 1;
    }
    // Determine file size
    fseek(encoded_snippet, 0, SEEK_END);
    long fileSize = ftell(encoded_snippet);
    fseek(encoded_snippet, 0, SEEK_SET);
    // Read the file contents into a vector
    std::vector<char> voiceData(fileSize);
    if (fread(voiceData.data(), 1, fileSize, encoded_snippet) != fileSize) {
        std::cerr << "Failed to read voice data into vector" << std::endl;
        return 1;
    }
    
    // Create a connection to the relay
    rpc::client client(RELAY_IP, 8080);
    std::this_thread::sleep_for(std::chrono::seconds(4));
    // Call the remote procedure to send the file
    client.call("sendVoice", voiceData);

    return 0;
}
