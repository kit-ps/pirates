//#include "params.h"
#include <cstdint>
#include <iostream>
#include <random>
#include <map>
#include <thread>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <unistd.h>
#include <openssl/aes.h>
#include "seal/seal.h"
#include <fstream>
#include <stack>
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/this_server.h>
#include <chrono>
#include "../FastPIR/src/fastpirparams.hpp"
#include "../FastPIR/src/server.hpp"
#include "../FastPIR/src/client.hpp"
#include "serialization_helper.h"
#include "logging_helper.h"

using namespace seal;

unsigned int NUM_COLUMNS;
int DB_ROWS;

std::string WORKER_IP = "";
std::string CALLEE_IP = "";
std::string RUN_ID = "";
int MESSAGE_SIZE = 1152;
int SNIPPET_SIZE;
int NUM_ROUNDS;
int NUM_USERS;
int GROUP_SIZE;
int NUM_THREAD = 12;
int NUM_BUCKET;

FastPIRParams *PIR_PARAMS;
Server *PIR_SERVER;
Client *PIR_CLIENT;
PIRQuery PIR_QUERY;
std::string PIR_SER_KEY;

Ciphertext *query;
Ciphertext *result;
GaloisKeys *gal_keys;
Plaintext *encoded_db;
BatchEncoder *batch_encoder;
Evaluator *evaluator;
Encryptor *encryptor;
seal::parms_id_type pid;
//std::shared_ptr<seal::SEALContext> context;
SEALContext *context;
KeyGenerator *keygen;
SecretKey secret_key;

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
//void *gen_keys(void *thread_id);
//void *pir(void *thread_id);
//void *preprocess_db(void *thread_id);

std::vector<uint8_t> compute_pir_reply() {
    auto reply = PIR_SERVER->get_response(0, PIR_QUERY);
    auto ser = seal_ser(reply);
    std::vector<uint8_t> result(ser.begin(), ser.end());
    return result;
}

/// Call compute_pir_reply count times and ignore return values
void compute_dummy_replies(int count) {
    for (int i = 0; i < count; i++) {
        compute_pir_reply();
    }
}

void process(int r, const std::vector<std::vector<uint8_t>>& raw_db) {
    
    uint64_t time_before_worker = get_time();

    std::cout << "Hi from worker" << std::endl;

    PIR_SERVER->set_db(raw_db);

    // 1. Preprocess raw database
    PIR_SERVER->preprocess_db();

    uint64_t time_after_preprocessing = get_time();

    // 2. Select a random index within all clients for the callee
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, NUM_USERS);
    int callee_index = distrib(gen);

    // Total number of replies up to callee: calle_index * number of buckets
    // Number of buckets: 1.5 * group size -1
    int replies_total = callee_index * NUM_BUCKET;
    int replies_per_thread = std::max(1.0, std::ceil(replies_total / NUM_THREAD));
    std::vector<std::thread> threads;
    
    // Span NUM_THREAD - 1 daughter threads and give each replies to compute
    for (int i = 0; i < NUM_THREAD - 1; i++) {
        threads.push_back(std::thread(compute_dummy_replies, replies_per_thread));
    }

    // Compute remaining replies in main thread
    std::vector<std::vector<uint8_t>> all_replies;
    for (int j = 0; j < replies_per_thread; j++) {
        std::vector<uint8_t> rep = compute_pir_reply();
        all_replies.push_back(rep);
    }

    std::vector<std::vector<uint8_t>> callee_replies(NUM_BUCKET, all_replies[0]);

    // Wait for other threads
    for (auto& t : threads) {
        t.join();
    }
    
    uint64_t time_after_replies = get_time();

    // 4. Send replies to callee
    rpc::client *client = new rpc::client(CALLEE_IP, 8080);
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client->call("process", r, PIR_SER_KEY, callee_replies);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            delete client;
            client = new rpc::client(CALLEE_IP, 8080);
        }
    }

    std::string log_content = RUN_ID + "-"
        + std::to_string(r) + ","
        + std::to_string(time_before_worker) + ","
        + std::to_string(time_after_preprocessing) + ","
        + std::to_string(time_after_replies);
        
    write_log("worker", log_content);
    if (r == NUM_ROUNDS - 1) {
        rpc::this_server().stop();
    }
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES worker ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "o:r:s:g:u:x:n:")) != -1) {
        switch(c) {
            case 'o':
                WORKER_IP = std::string(optarg);
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
                CALLEE_IP = std::string(optarg);
                break;
            default:
                abort();
        }
    }

    NUM_BUCKET = std::ceil(1.5 * (GROUP_SIZE - 1));
    int items_per_bucket = std::ceil(3 * NUM_USERS / NUM_BUCKET);
    PIR_PARAMS = new FastPIRParams(items_per_bucket, SNIPPET_MAP[SNIPPET_SIZE]);
    PIR_SERVER = new Server(*PIR_PARAMS);
    PIR_CLIENT = new Client(*PIR_PARAMS);
    PIR_SERVER->set_client_galois_keys(0, PIR_CLIENT->get_galois_keys());
    PIR_QUERY = PIR_CLIENT->gen_query(0); 
    PIR_SER_KEY = seal_ser(PIR_CLIENT->get_secret_key());

    // Create database
    //raw_db = new
    // Receive data from relay
    // Create an RPC server
    rpc::server server(WORKER_IP, 8080);

    // Bind the sendVoice function to a remote procedure
    server.bind("process", process);
    server.run();

    return 0;
}
