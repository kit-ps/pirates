#include <cstdint>
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
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/rng.h>
#include <botan/secmem.h>
#include "serialization_helper.h"
#include "logging_helper.h"
#include "../FastPIR/src/fastpirparams.hpp"
#include "../FastPIR/src/client.hpp"

std::string CALLEE_IP = "";
std::string RUN_ID = "";
int MESSAGE_SIZE = 1152;
int SNIPPET_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int NUM_USERS;
int GROUP_SIZE;
int NUM_CLIENTS = 512;

/// Gets as input a PIR reply and returns the contained snippet
std::vector<uint8_t> process_reply(std::vector<uint8_t> rep, Client& pir_client) {
    std::string rep_str(rep.begin(), rep.end());
    auto cipher = seal_deser<seal::Ciphertext>(rep_str, *pir_client.get_seal_context());
    return pir_client.decode_response(cipher, 0);
}

// AES-decrypts the given snippet and returns the resulting plaintext
std::vector<uint8_t> decrypt_reply(std::vector<uint8_t> snippet) {
    const std::vector<uint8_t> key = Botan::hex_decode("C0FFEEC0FFEC0FFEEC0FFEC0FFEEC0FF");

    auto dec = Botan::Cipher_Mode::create("AES-128/CBC/PKCS7", Botan::DECRYPTION);
    dec->set_key(key);

    // generate 0 nonce (IV)
    Botan::secure_vector<uint8_t> iv(0, dec->default_nonce_length());

    // Copy input data to a buffer that will be encrypted
    Botan::secure_vector<uint8_t> decrypted_snippet (
            snippet.begin(), 
            snippet.end());

    dec->start(iv);
    dec->finish(decrypted_snippet);

    return Botan::unlock(decrypted_snippet);
}

std::vector<short> decode_reply(std::vector<uint8_t> snippet) {
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

    int num_frames = snippet.size() / lpcnet_bits_per_frame(lf);
    for (int i = 0; i < num_frames; i++) {
        // copy from reply vector into frame
        std::copy(
                snippet.begin() + i * lpcnet_bits_per_frame(lf),
                snippet.begin() + (i+1) * lpcnet_bits_per_frame(lf), 
                frame);

        lpcnet_dec(lf,frame,pcm);
        for (short s : pcm) {
            decoded_snippet.push_back(s);
        }
    }

    return decoded_snippet;
}

void process(int r, const std::string& secret_key, const std::vector<std::vector<uint8_t>>& replies) {
    std::string log_content = RUN_ID + '-' + std::to_string(r);
    std::cout << "Hi from callee" << std::endl;
    // Declare data type for reply vector
    std::vector<std::vector<uint8_t>> pir_decoded_replies;
    std::vector<std::vector<uint8_t>> aes_decrypted_replies;
    std::vector<std::vector<short>> lpc_decoded_replies;
    uint64_t time_before_callee = get_time();
    log_content += std::to_string(time_before_callee) + ",";
    FastPIRParams pir_params(NUM_CLIENTS, MESSAGE_SIZE);
    Client pir_client(pir_params, seal_deser<seal::SecretKey>(secret_key, seal::SEALContext(pir_params.get_seal_params())));

    // TODO: Split up loop so we can get time for decode/decrypt/decode separately
    //std::vector<uint8_t> rep;
    // PIR process reply
    // threads
    /*
    for (int i = 0; i < GROUP_SIZE - 1; i++) {
        rep = process_reply(rep, pir_client);
        std::cout << "Reply length: " << rep.size() << std::endl;
    }
    */
    for (std::vector<uint8_t> reply: replies) {
        pir_decoded_replies.push_back(process_reply(reply, pir_client));
    }
    // barrier
    log_content += std::to_string(get_time()) + ',';
    // AES decrypt reply
    for (std::vector<uint8_t> reply: pir_decoded_replies) {
        aes_decrypted_replies.push_back(decrypt_reply(reply));
    }

    std::cout << "I am showing you the start of the encoded snippet: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << ((int)aes_decrypted_replies[0][i]) << " ";
    }
    std::cout << std::dec << std::endl;

    log_content += std::to_string(get_time());
    

    // LPCNet decode reply
    for (std::vector<uint8_t> reply: aes_decrypted_replies) {
        lpc_decoded_replies.push_back(decode_reply(reply));
    }

    uint64_t time_after_callee = get_time();
    log_content += std::to_string(time_after_callee);
    // time before callee
    // time after PIR
    // time after decryption
    // time after LPC
    write_log("callee", log_content);
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES client ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "o:r:s:g:u:x:")) != -1) {
        switch(c) {
            case 'o':
                CALLEE_IP = std::string(optarg);
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
