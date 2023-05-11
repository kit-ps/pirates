#include <iostream>
#include <chrono>
#include <iterator>
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

void process() {
    //////////////////////////////////////////// LPCNet Encoding ////////////////////////////////////////////
    std::cout << "Encoding voice snippet ..." << std::endl;
    // Open input file
    FILE *input_file;
    std::string tmp = "/audio/raw_" + std::to_string(SNIPPET_SIZE) + "ms.wav";
    input_file = fopen(tmp.c_str(), "rb");
    if (!input_file) {
        std::cerr << "Failed to open raw audio file!" << std::endl;
        return;
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
    //std::cout << "Needed time for LPCNet encoding: " << duration.count() << "\u03bcs\n";
    std::cout << "Needed time for LPCNet encoding: " << duration.count() << "mus" << std::endl;
     // Close files for encoding
    fclose(input_file);

    //////////////////////////////////////////// AES ENCRYPTION  ////////////////////////////////////////////
    std::cout << "Encrypting encoded voice snippet ..." << std::endl;
    // For the moment, key taken from Addra
    unsigned char key[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 256, &aes_key);
    const int key_length = 256;
    unsigned char iv[AES_BLOCK_SIZE];
    // For now, we set the IV to 0x00
    memset(iv, 0x00, AES_BLOCK_SIZE);
    // Define files
    //std::ifstream in("/home/app/encoded_audio", std::ios::binary);
    //std::ofstream out("/home/app/encrypted_audio", std::ios::binary);
    // Get file size
    //in.seekg(0, std::ios::end);
    //const auto file_size = in.tellg();
    //in.seekg(0, std::ios::beg);
    //unsigned char buffer[16];
    //unsigned char encrypted_buffer[16];
    //
    start = std::chrono::high_resolution_clock::now();


    
    // copy encoded_snippet into buffer
    std::copy(encoded_snippet.begin(), encoded_snippet.end(), buffer);

    AES_cbc_encrypt(encoded_snippet.data(), encrypted_buffer, encoded_snippet.size(), &aes_key, iv, AES_ENCRYPT);

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Needed time for AES encryption: " << duration.count() << "mus\n";

    // convert buffer to vector
    std::vector<char> encrypted_snippet;
    // naive because c++11 does not work :(
    for (int i = 0; i < encoded_snippet.size(); i++) {
        encrypted_snippet.push_back(encrypted_buffer[i]);
    }
    //std::vector<unsigned char> encrypted_snippet(std::begin(encrypted_buffer), std::end(encrypted_buffer));

   
    /*
    // determine number of blocks needed 
    int num_blocks;
    float tmp_num_blocks = encoded_snippet.size() / AES_BLOCK_SIZE;
    if (floorf(tmp_num_blocks) == tmp_num_blocks) {
        // encoded snipped can be perfectly divided into blocks
        num_blocks = static_cast<int>(tmp_num_blocks);
    } else {
        // padding is needed
        num_blocks = static_cast<int>(tmp_num_blocks) + 1; 

    }

    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char encrypted_buffer[AES_BLOCK_SIZE];
    std::vector<char> encrypted_snippet;

    std::cout << "Number of blocks needed: " << tmp_num_blocks << " -> " << num_blocks << std::endl;

    for (int i = 0; i < num_blocks; i++) {
        std::fill(buffer, buffer+AES_BLOCK_SIZE, '0');
        if ((i+1)*AES_BLOCK_SIZE <= encoded_snippet.size()) {
            // full buffer can be taken from snippet
            std::copy(
                    encoded_snippet.begin() + i*AES_BLOCK_SIZE,
                    encoded_snippet.begin() + (i+1)*AES_BLOCK_SIZE,
                    buffer
                    );
        } else {
            // buffer has to be padded 
            std::copy(
                    encoded_snippet.begin() + i*AES_BLOCK_SIZE,
                    encoded_snippet.end(),
                    buffer
                    );
        }
        // encrypt buffer to encrypted_buffer
        AES_cbc_encrypt(buffer, encrypted_buffer, 16, &aes_key, iv, AES_ENCRYPT);

        // append encrypted_buffer to encrypted_snippet
        for (int j = 0; j < AES_BLOCK_SIZE; j++) {
            encrypted_snippet.push_back(encrypted_buffer[j]);
        }
    }
    */

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Needed time for AES encryption: " << duration.count() << "mus\n";
    /*
    while (in.read(reinterpret_cast<char*>(buffer), 16)) {
        // Encrypt the 16-byte buffer and write to output file
        AES_cbc_encrypt(buffer, encrypted_buffer, 16, &aes_key, iv, AES_ENCRYPT);
        out.write(reinterpret_cast<const char*>(encrypted_buffer), 16);
    }

    // Add padding to the last block if necessary
    if (in.gcount() != 0) {
        std::fill(std::begin(buffer) + in.gcount(), std::end(buffer), 0);
        AES_cbc_encrypt(buffer, encrypted_buffer, 16, &aes_key, iv, AES_ENCRYPT);
        out.write(reinterpret_cast<const char*>(encrypted_buffer), 16);
    }
    */
   

    //////////////////////////////////////////// Send to Relay ////////////////////////////////////////////
    // Create a connection to the relay
    rpc::client client(RELAY_IP, 8080);
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client.call("process", encoded_snippet);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    // Call the remote procedure to send the file
    //client.call("process", encoded_snippet);
}

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
    
    for (int i = 0; i < NUM_ROUNDS; i++) {
        process();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
