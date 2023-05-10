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
#include <rpc/server.h>

std::string CALLEE_IP = "";
std::string MASTER_IP = "";
std::string RELAY_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int GROUP_SIZE;

std::vector<char> process_reply(std::vector<char> rep) {
    // TODO
    return rep;
}

std::vector<char> decrypt_reply(std::vector<char> rep) {
    // TODO
    return rep;
}

std::vector<short> decode_reply(std::vector<char> rep) {
    // Taken from lpcnet_dec.c
    int nbits = 0, nerrs = 0;
    // Default in lpcnet_dec is 0.0
    float ber = 0.0;
    // Default in lpcnet_dec is -1
    int ber_en = -1;
    // Default in lpcnet_dec is 0
    int ber_st = 0;
    // We decode
    LPCNetFreeDV *lf = lpcnet_freedv_create(0);
    // Taken from lpcnet_enc.c
    char frame[lpcnet_bits_per_frame(lf)];
    short pcm[lpcnet_samples_per_frame(lf)];

    std::vector<short> decoded_snippet;

    int num_frames = rep.size() / lpcnet_bits_per_frame(lf);
    for (int i = 0; i < num_frames; i++) {
        // copy from reply vector into frame
        std::copy(
                rep.begin() + i * lpcnet_bits_per_frame(lf),
                rep.begin() + (i+1) * lpcnet_bits_per_frame(lf), 
                frame);

        lpcnet_dec(lf,frame,pcm);
        for (short s : pcm) {
            decoded_snippet.push_back(s);
        }
    }

    return decoded_snippet;
}

void process(const std::vector<char>& replies) {
    std::cout << "Hi from callee" << std::endl;

    // Compute size of single reply
    int rep_size = replies.size() / GROUP_SIZE - 1; 

    std::cout << "Rep size: " << rep_size << std::endl;

    std::vector<char> rep;
    for (int i = 0; i < GROUP_SIZE - 1; i++) {
        // get current reply from replies.
        rep = std::vector<char>(
                replies.begin() + i * rep_size, 
                replies.begin() + (i + 1) * rep_size); 

        // PIR process reply
        rep = process_reply(rep);
        
        // AES decrypt reply
        rep = decrypt_reply(rep);

        // LPCNet decode reply
        std::vector<short> decoded_snippet = decode_reply(rep);
    }
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES client ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "c:m:s:n:r:e:g:")) != -1) {
        switch(c) {
            case 'c':
                CALLEE_IP = std::string(optarg);
                std::cout << "Callee IP is " << CALLEE_IP << std::endl;
                break;
            case 'e':
                RELAY_IP = std::string(optarg);
                break;
            case 's':
                MESSAGE_SIZE = std::stoi(optarg);
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
    std::ofstream logFile("callee_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }
    // Create an RPC server
    rpc::server server(CALLEE_IP, 8080);

    // Bind the process function to a remote procedure
    server.bind("process", process);

    // Run the server
    //server.async_run(1);
    server.run();
    // Close log file
    logFile.close();
    return 0;
    
    /*
    //////////////////////////////////////////// LPCNET DECODING ////////////////////////////////////////////
    
    std::cout << "Decoding voice snippet ..." << std::endl;
    // Open files
    fin_dec = fopen("/home/app/pirates/birch_encoded", "rb");
    fout_dec = fopen("/home/app/pirates/birch_decoded.wav", "wb");
    // Taken from lpcnet_dec.c
    int nbits = 0, nerrs = 0;
    int bits_read = 0;
    // Default in lpcnet_dec is 0.0
    float ber = 0.0;
    // Default in lpcnet_dec is -1
    int ber_en = -1;
    // Default in lpcnet_dec is 0
    int ber_st = 0;
    // We decode
    start = std::chrono::high_resolution_clock::now();
    do {
        bits_read = fread(frame, sizeof(char), lpcnet_bits_per_frame(lf), fin_dec);
        nbits += ber_en - ber_st;
        if (ber != 0.0) {
            int i;
            for(i=ber_st; i<=ber_en; i++) {
                float r = (float)rand()/RAND_MAX;
                if (r < ber) {
                    frame[i] = (frame[i] ^ 1) & 0x1;
                    nerrs++;
                }
            }
        }            

        lpcnet_dec(lf,frame,pcm);
        fwrite(pcm, sizeof(short), lpcnet_samples_per_frame(lf), fout_dec);
        
        if (fout_dec == stdout) fflush(stdout);
        
    } while(bits_read != 0);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Needed time for LPCNet decoding: " << duration.count() << "\u03bcs\n";
    logFile.close();
    return 0;
    */
}
