#include <iostream>
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
#include <boost/asio.hpp>
#include<stack>

using namespace seal;
using boost::asio::ip::tcp;

#define N 4096
#define PLAIN_BIT 19
#define COEFF_MODULUS_54 18014398509309953UL
#define COEFF_MODULUS_55 36028797018652673UL
#define PLAIN_MODULUS 270337
#define NUM_CT_PER_QUERY (2 * (NUM_MESSAGE / N))
#define SUBROUND_TIME 480 // In miliseconds
#define RAW_DB_SIZE (MESSAGE_SIZE * NUM_MESSAGE)
#define SERVER_PACKET_SIZE (8 * 1024)
#define CLIENT_PACKET_SIZE (64 * 1024)
#define CT_SIZE (2 * 8 * N) //In Bytes
#define OPTIMIZATION_2
#define MIN(a,b) ((a) < (b)) ? (a) : (b)

#define NTT_NUM_THREAD 8

#define MASTER_PORT 3000
#define WORKER_PORT 2199
#define CLIENT_PORT 2000

std::string RELAY_IP = "";
std::string WORKER_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int NUM_CLIENT;
int GROUP_SIZE;
int NUM_THREAD = 0;
char *raw_db;

Ciphertext **query;
Ciphertext *result;
GaloisKeys *gal_keys;
Plaintext *encoded_db;
BatchEncoder *batch_encoder;
Evaluator *evaluator;
Encryptor *encryptor;
seal::parms_id_type pid;
std::shared_ptr<seal::SEALContext> context;
KeyGenerator *keygen;
SecretKey secret_key;

void *gen_keys(void *thread_id);
void *pir(void *thread_id);
void *preprocess_db(void *thread_id);

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES worker ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "m:w:s:n:r:g:")) != -1) {
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
    // Receive data from relay
    boost::asio::io_context io_context;
    // Create a TCP acceptor to listen for incoming connections.
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));
    // Wait for incoming connection(s) and handle them.
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Received connection from " << socket.remote_endpoint().address().to_string() << ":" << socket.remote_endpoint().port() << std::endl;
        // Do something with the incoming connection here.
        // Open the file
        std::string file_name = "/home/app/forwarded_audio";
        std::ofstream file(file_name, std::ios::binary);

        if (!file) {
            std::cerr << "Could not create file " << file_name << std::endl;
            return 1;
        }
        // Receive the file size from the client
        size_t file_size;
        boost::asio::read(socket, boost::asio::buffer(&file_size, sizeof(file_size)));

        // Receive the file from the client
        char buffer[1024];
        size_t total_received = 0;
        while (total_received < file_size) {
            size_t received = socket.read_some(boost::asio::buffer(buffer, std::min(file_size - total_received, sizeof(buffer))));
            total_received += received;
        }

        // Close the file and the socket
        file.close();
        socket.close();
    }
    // Close log file
    logFile.close();
    return 0;
}

// Taken from Addra
void *pir(void *thread_id) {  

    Ciphertext column_sum, temp_ct;
    column_sum = query[0][0]; 
    temp_ct = query[0][0]; 
    int client_factor = floor((double)NUM_CLIENT/NUM_THREAD);
    int start_id = *((int *)thread_id) * client_factor;
    int remaining = NUM_CLIENT % NUM_THREAD;
    if(remaining > *((int *)thread_id)) {
        start_id += *((int *)thread_id);
        client_factor++;
    }else {
        start_id += remaining;
    }
    int end_id = MIN((start_id + client_factor), NUM_CLIENT);
    std::stack<Ciphertext> st;
    int my_round = 1;
    std::vector<uint64_t> ct(2*N);

    for(int client_id = start_id; client_id < end_id ; client_id++) {
        result[client_id] = query[client_id][0]; // TO allocate memory
        if((client_id - active_client_start)%active_client_interval == 0) {
            result[client_id] = query[client_id][0];
            std::copy(result[client_id].data(), result[client_id].data() +(2 * N), ct.begin());
            active_clients[((client_id - active_client_start)/active_client_interval)]->async_call("sendCT",ct);
        }
 
    }

    while(my_round <= NUM_ROUND) { 
        pthread_mutex_lock(&pir_lock);
        while (pir_round != my_round) {     
            pthread_cond_wait(&pir_cond, &pir_lock);
        }
        pthread_mutex_unlock(&pir_lock);

        // Compact implementation of FastPIR functionalities
   
        for(int client_id = start_id; client_id < end_id; client_id++) {
            for(int i = 0; i < NUM_COLUMNS/2;i++) {
                evaluator->multiply_plain(query[client_id][0], encoded_db[NUM_CT_PER_QUERY*i ], column_sum);
                for(int j = 1; j < NUM_CT_PER_QUERY; j++) {
                    evaluator->multiply_plain(query[client_id][j], encoded_db[NUM_CT_PER_QUERY*i + j], temp_ct);
                    evaluator->add_inplace(column_sum, temp_ct);
                }
                evaluator->transform_from_ntt_inplace(column_sum);

                // Iterative implementation of FastPIR's tree optimization

                int k = i+1;
                int step_size = -1;

                while(k%2 == 0) {
                    Ciphertext xx = st.top();
                    st.pop();
                    evaluator->rotate_rows_inplace(column_sum, step_size, gal_keys[client_id]);
                    evaluator->add_inplace(column_sum, xx);
                    k /= 2;
                    step_size *= 2;
                }
                st.push(column_sum);
            }        
            result[client_id] = st.top();
            st.pop();
            int mask = NUM_COLUMNS/2;
            mask &= (mask -1); // Reset rightmost bit
            while(mask && st.size()) {
                int step_size = - (mask & ~(mask-1)); // Only setting the rightmost 1
                evaluator->rotate_rows_inplace(result[client_id], step_size, gal_keys[client_id]);
                Ciphertext xx = st.top();
                st.pop();
                evaluator->add_inplace(result[client_id], xx);
                mask &= (mask -1); // Reset rightmost bit
            }
        }
        ++my_round;
        pthread_barrier_wait(&pir_barrier);

        for(int client_id = start_id; client_id < end_id ; client_id++) {
            if((client_id - active_client_start)%active_client_interval == 0) {
                std::copy(result[client_id].data(), result[client_id].data() +(2 * N), ct.begin());
                active_clients[((client_id - active_client_start)/active_client_interval)]->async_call("sendCT",ct);
            }
        }

    }

    //sleep(5);

    return 0;
}

// Taken from Addra
void *preprocess_db(void *thread_id) {

    int client_factor = floor((double)DB_ROWS/NTT_NUM_THREAD);
    int start_id = *((int *)thread_id) * client_factor;
    int remaining = DB_ROWS % NTT_NUM_THREAD;
    if(remaining > *((int *)thread_id)) {
        start_id += *((int *)thread_id);
        client_factor++;
    } else {
        start_id += remaining;
    }

    int end_id = MIN((start_id + client_factor), DB_ROWS);
    vector<uint64_t> db_element(N);
    int my_round = 1;

    while(my_round <= NUM_ROUND) {
        pthread_mutex_lock(&preprocess_lock);
        while (preprocessing_round != my_round) {     
            pthread_cond_wait(&preprocess_cond, &preprocess_lock);
        }
        pthread_mutex_unlock(&preprocess_lock);

        for(int row = start_id;row < end_id;row++) {
            for(int j = 0; j < N; j++) {
                //db_element[j] = rand()%(1<<(PLAIN_BIT-1));
                db_element[j] = raw_db[j & (RAW_DB_SIZE -1)]+row + my_round * 7;
            }

            encoded_db[row].release();
            batch_encoder->encode(db_element, encoded_db[row]);
            evaluator->transform_to_ntt_inplace(encoded_db[row], pid);
        }
        ++my_round;
        pthread_barrier_wait(&preprocess_barrier);
    }

    return 0;
}
