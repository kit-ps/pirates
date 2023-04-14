#include <iostream>
#include <fstream>
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
#include <boost/asio.hpp>

std::string MASTER_IP = "";
std::string WORKER_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;

using boost::asio::ip::tcp;

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES master ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "m:w:s:n:r:")) != -1) {
        switch(c) {
            case 'm':
                MASTER_IP = std::string(optarg);
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
            default:
                abort();
        }
    }

    //////////////////////////////////////////// RECEIVING VOICE SNIPPETS /////////////////////////////////////
    
    return 0;
}
