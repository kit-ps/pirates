//#include "params.h"
#include <cstdint>
#include <iostream>
#include <random>
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
int NUM_THREAD = 0;
FastPIRParams *PIR_PARAMS;
//char *raw_db;

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

//void *gen_keys(void *thread_id);
//void *pir(void *thread_id);
//void *preprocess_db(void *thread_id);

std::vector<uint8_t> compute_pir_reply(Server& pir_server, Client& pir_client) {
    auto query = pir_client.gen_query(0);
    pir_server.set_client_galois_keys(0, pir_client.get_galois_keys());
    auto reply = pir_server.get_response(0, query);
    auto ser = seal_ser(reply);
    std::vector<uint8_t> result(ser.begin(), ser.end());
    return result;
}

void process(int r, const std::vector<std::vector<uint8_t>>& raw_db) {
    
    uint64_t time_before_worker = get_time();

    std::cout << "Hi from worker" << std::endl;

    PIR_PARAMS = new FastPIRParams(raw_db.size(), raw_db[0].size());
    Server pir_server(*PIR_PARAMS);
    Client pir_client(*PIR_PARAMS);
    auto serialized_secret_key = seal_ser(pir_client.get_secret_key());

    /*
    std::vector<std::vector<uint8_t>> unrolled_db;
    for (int i = 0; i < NUM_USERS; i++)
    {
        std::vector<uint8_t> current(MESSAGE_SIZE);
        std::copy(raw_db.begin() + i * MESSAGE_SIZE, raw_db.begin() + (i + 1) * MESSAGE_SIZE, current.begin());
        unrolled_db.push_back(current);
    }
    pir_server.set_db(unrolled_db);
    */
    pir_server.set_db(raw_db);
    std::cout << "SET DB" << std::endl;

    // 1. Preprocess raw database
    pir_server.preprocess_db();
    std::cout << "PREPROCESS DB" << std::endl;

    uint64_t time_after_preprocessing = get_time();

    // 2. Select a random index within all clients for the callee
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, NUM_USERS);
    int callee_index = distrib(gen);

    // 3. Generate GROUP_SIZE PIR answers up to callee index
    std::vector<std::vector<uint8_t>> replies;
    for (int j = 0; j < callee_index; j++) {
        replies.clear();
        for (int k = 0; k < 1.5 * (GROUP_SIZE - 1); k++) {
            std::vector<uint8_t> rep = compute_pir_reply(pir_server, pir_client);
            //replies.insert(std::end(replies), std::begin(rep), std::end(rep));
            replies.push_back(rep);
        }
    }
    
    uint64_t time_after_replies = get_time();

    // 4. Send replies to callee
    rpc::client *client = new rpc::client(CALLEE_IP, 8080);
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client->call("process", r, serialized_secret_key, replies);
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
