//#include <params.h>
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

#define NTT_NUM_THREAD 8
#define PLAIN_BIT 19
#define N 4096
#define NUM_CT_PER_QUERY (2 * (NUM_MESSAGE / N))
#define COEFF_MODULUS_54 18014398509289953UL
#define COEFF_MODULUS_55 36028797018652673UL
#define PLAIN_MODULUS 270337

using namespace seal;

unsigned int NUM_COLUMNS;
int DB_ROWS;

pthread_t *threads;
pthread_t ntt_threads[NTT_NUM_THREAD];
int *thread_id;

std::string RELAY_IP = "";
std::string WORKER_IP = "";
std::string CALLEE_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int NUM_CLIENT;
int GROUP_SIZE;
int NUM_THREAD = 0;
//char *raw_db;

Ciphertext **query;
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

/// Preprocess the raw database received from the relay
/// to prepare for answering PIR requests.
std::vector<uint8_t> preprocess_db(std::vector<uint8_t> raw_db) {
    // TODO
    return raw_db;
}

/// Compute a single PIR reply on the preprocessed db
std::vector<uint8_t> compute_pir_reply(std::vector<uint8_t> preprocessed_db) {
    // TODO
    return {'a', 'b', 'c', 'd', 'e'};
}

void process(const std::vector<uint8_t>& raw_db) {
    
    std::cout << "Hi from worker" << std::endl;

    // 1. Preprocess raw database
    std::vector<uint8_t> preprocessed_db = preprocess_db(raw_db);

    // 2. Select a random index within all clients for the callee
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, NUM_CLIENT);
    int callee_index = distrib(gen);

    // 3. Generate GROUP_SIZE PIR answers up to callee index
    std::vector<uint8_t> replies;
    for (int j = 0; j < callee_index; j++) {
        replies = {};
        for (int k = 0; k < GROUP_SIZE - 1; k++) {
            std::vector<uint8_t> rep = compute_pir_reply(preprocessed_db);
            replies.insert(std::end(replies), std::begin(rep), std::end(rep));
        }
    }

    // 4. Send replies to callee
    rpc::client client(CALLEE_IP, 8080);
    // Retry connection until the server is available
    bool success = false;
    while (!success) {
        try {
            // Attempt to connect to the server
            client.call("process", replies);
            success = true;
        } catch (const std::exception& e) {
            // Connection failed, sleep for a while before retrying
            std::cout << "Retrying" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES worker ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "c:w:s:n:r:g:u:")) != -1) {
        switch(c) {
            case 'w':
                WORKER_IP = std::string(optarg);
                break;
            case 'c':
                CALLEE_IP = std::string(optarg);
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
            case 'u':
                NUM_CLIENT = std::stoi(optarg);
                break;
            case 't':
                NUM_THREAD = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }
    // Create a file for logging
    std::ofstream logFile("worker_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }
    // Number of columns for the database
    NUM_COLUMNS = (ceil((double)(MESSAGE_SIZE * 4) / (PLAIN_BIT - 1)) * 2);
    // Number of rows
    DB_ROWS = (NUM_CT_PER_QUERY * (NUM_COLUMNS/2));
    // Create threats for calculating the PIR answer
    threads = new pthread_t [NUM_THREAD];
    thread_id = new int[NUM_THREAD];
    srand(time(NULL));
    //EncryptionParameters parms(scheme_type::BFV);
    EncryptionParameters parms(scheme_type::bfv);
    parms.set_poly_modulus_degree(N);
    //parms.set_coeff_modulus({COEFF_MODULUS_54, COEFF_MODULUS_55});
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(N));
    parms.set_plain_modulus(PLAIN_MODULUS);
    std::cout << "N: ";
    std::cout << N << std::endl;
    std::cout << "COEFF_MODULUS_54: ";
    std::cout << COEFF_MODULUS_54 << std::endl;
    std::cout << "COEFF_MODULUS_55: ";
    std::cout << COEFF_MODULUS_55 << std::endl;
    std::cout << "PLAIN_MODULUS: ";
    std::cout << PLAIN_MODULUS << std::endl;
    context = new SEALContext(parms);
    std::cout << "Parameter validation (success): " << context->parameter_error_message() << std::endl;    
    //print_parameters(context);

    //std::cout << NUM_MESSAGE << std::endl;
    //batch_encoder = new BatchEncoder(*context);
    KeyGenerator keygen(*context);
    //evaluator = new Evaluator(context);
    //encoded_db = new Plaintext[DB_ROWS];
    //result = new Ciphertext[NUM_CLIENT];
    // Create database
    //raw_db = new
    // Receive data from relay
    // Create an RPC server
    rpc::server server(WORKER_IP, 8080);

    // Bind the sendVoice function to a remote procedure
    server.bind("process", process);
    server.run();
    // Close log file
    logFile.close();
    return 0;
}