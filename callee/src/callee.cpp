#include <cstdint>
#include <iostream>
#include <chrono>
#include <map>
#include <thread>
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
#include <rpc/this_server.h>
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/rng.h>
#include <botan/secmem.h>
#include <mutex>
#include <condition_variable>
#include <future>
#include <boost/asio.hpp>
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
int NUM_THREAD = 8;
boost::asio::thread_pool *pool;

std::map<int, int> SNIPPET_MAP = {
    { 40, 368 },
    { 60, 576 },
    { 80, 736 },
    { 100, 944 },
    { 120, 1152 },
    { 140, 1312 },
    { 160, 1520 },
    { 180, 1728 },
    { 200, 1888 },
    { 220, 2096 },
    { 240, 2304 },
    { 260, 2448 },
    { 280, 2656 },
    { 300, 2864 }
};

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
    std::mutex m;
    std::condition_variable cv;
    int completed = 0;
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


    int bpf = lpcnet_bits_per_frame(lf);
    int spf = lpcnet_samples_per_frame(lf);

    int num_frames = snippet.size() / bpf;
    std::vector<short> decoded_snippet(spf * num_frames, 0);

    for (int i = 0; i < num_frames; i++) {
        boost::asio::post(*pool, [&completed,&m,&cv,&snippet,i,bpf,spf,&decoded_snippet,lf] {
            // Taken from lpcnet_enc.c
            char frame[bpf];
            short pcm[spf];


            // copy from reply vector into frame
            std::copy(snippet.begin() + i * bpf, snippet.begin() + (i+1) * bpf, frame);
            lpcnet_dec(lf,frame,pcm);

            for (int j = 0; j < spf; ++j) {
                decoded_snippet[i * spf + j] = pcm[j];
            }

            {
                std::unique_lock lk(m);
                completed++;
            }
            cv.notify_all();
        });
    }

    // Wait for other threads
    {
        std::unique_lock lk(m);
        cv.wait(lk, [&completed,num_frames] { return completed == num_frames; });
    }

    //lpcnet_freedv_destroy(lf);

    return decoded_snippet;
}


void process(int r, const std::string& secret_key, const std::vector<std::vector<uint8_t>>& replies) {
    std::string log_content = RUN_ID + '-' + std::to_string(r) + ',';
    std::cout << "Hi from callee" << std::endl;

    std::mutex m;
    std::condition_variable cv;
    int completed = 0;

    // Declare data type for reply vector
    std::vector<std::vector<uint8_t>> aes_decrypted_replies;
    uint64_t time_before_callee = get_time();
    log_content += std::to_string(time_before_callee) + ",";

    int num_bucket = std::ceil(1.5 * (GROUP_SIZE - 1));
    int items_per_bucket = std::ceil(3 * NUM_USERS / num_bucket);
    FastPIRParams pir_params(items_per_bucket, SNIPPET_MAP[SNIPPET_SIZE]);
    Client pir_client(pir_params, seal_deser<seal::SecretKey>(secret_key, seal::SEALContext(pir_params.get_seal_params())));

    std::promise<std::vector<uint8_t>> pir_decoded_reply_promise;
    auto pir_decoded_reply_future = pir_decoded_reply_promise.get_future();
        
    for (int i= 0; i < replies.size() - 1; ++i) {
        boost::asio::post(*pool, [&m,&cv,&completed,&replies,&pir_client] {
            process_reply(replies[0], pir_client);
            {
                std::unique_lock lk(m);
                completed++;
            }
            cv.notify_all();
        });
    }

    boost::asio::post(*pool, std::bind([&replies,&pir_client] (std::promise<std::vector<uint8_t>>& pir_decoded_reply_promise) {
        pir_decoded_reply_promise.set_value(process_reply(replies[0], pir_client));
    }, std::move(pir_decoded_reply_promise)));

    std::vector<std::vector<uint8_t>> pir_decoded_replies(replies.size(), pir_decoded_reply_future.get());

    // Wait for other threads
    {
        std::unique_lock lk(m);
        cv.wait(lk, [&completed,&replies] { return completed == replies.size() - 1; });
    }
    completed = 0;

    log_content += std::to_string(get_time()) + ',';

    // AES decrypt reply
    for (std::vector<uint8_t> reply: pir_decoded_replies) {
        aes_decrypted_replies.push_back(decrypt_reply(reply));
    }

    log_content += std::to_string(get_time()) + ',';

    // LPCNet decode reply
    std::promise<std::vector<short>> lpc_decoded_reply_promise;
    auto lpc_decoded_reply_future = lpc_decoded_reply_promise.get_future();
        
    for (int i= 0; i < replies.size() - 1; ++i) {
        boost::asio::post(*pool, [&m,&cv,&completed,&aes_decrypted_replies] {
            decode_reply(aes_decrypted_replies[0]);
            {
                std::unique_lock lk(m);
                completed++;
            }
            cv.notify_all();
        });
    }

    boost::asio::post(*pool, std::bind([&aes_decrypted_replies] (std::promise<std::vector<short>>& lpc_decoded_reply_promise) {
        lpc_decoded_reply_promise.set_value(decode_reply(aes_decrypted_replies[0]));
    }, std::move(lpc_decoded_reply_promise)));

    std::vector<std::vector<short>> lpc_decoded_replies(replies.size(), lpc_decoded_reply_future.get());

    // Wait for other threads
    {
        std::unique_lock lk(m);
        cv.wait(lk, [&completed,&replies] { return completed == replies.size() - 1; });
    }

    uint64_t time_after_callee = get_time();
    log_content += std::to_string(time_after_callee);
    // time before callee
    // time after PIR
    // time after decryption
    // time after LPC
    write_log("callee", log_content);
    if (r == NUM_ROUNDS - 1) {
        rpc::this_server().stop();
    }
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

    pool = new boost::asio::thread_pool(NUM_THREAD);

    // Create an RPC server
    rpc::server server(CALLEE_IP, 8080);

    // Bind the process function to a remote procedure
    server.bind("process", process);

    // Run the server
    server.run();
    
}
