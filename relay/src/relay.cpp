#include <iostream>
#include <array>
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

std::string RELAY_IP = "";
std::string WORKER_IP = "";
int MESSAGE_SIZE;
int NUM_MESSAGE;
int NUM_ROUNDS;
int GROUP_SIZE;

using boost::asio::ip::tcp;

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES relay ..." << std::endl;
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
            default:
                abort();
        }
    }
    // Create database
    unsigned char db[NUM_MESSAGE];
    // Create a file for logging
    std::ofstream logFile("relay_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open log file!" << std::endl;
        size_t file_size;
    boost::asio::read(socket, boost::asio::buffer(&file_size, sizeof(file_size)));
        return 1;
    }
    // Receive data from client
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
    // Send data to Worker
    std::ifstream ifs_out("/home/app/forwarded_audio", std::ios::binary);
    if (!ifs_out) {
        std::cerr << "Failed to open forwarded audio." << std::endl;
        return 1;
    }
    boost::asio::io_context io_context_out;
    tcp::resolver resolver_out(io_context_out);
    bool connected = false;
    while (!connected) {
        try {
            tcp::resolver::results_type endpoints_out = resolver_out.resolve(WORKER_IP, "8080");
            tcp::socket socket_out(io_context_out);
            boost::asio::connect(socket_out, endpoints_out);
            boost::system::error_code error_out;
            while (ifs_out.good()) {
                char buf[1024];
                ifs_out .read(buf, sizeof(buf));
                auto bytes_read = ifs_out.gcount();
                if (bytes_read > 0) {
                    boost::asio::write(socket_out, boost::asio::buffer(buf, bytes_read), error_out);
                    if (error_out) {
                        std::cerr << "Error sending file: " << error_out.message() << std::endl;
                        break;
                    }
                }
            }
            connected = true;
        } catch (const boost::system::system_error& e) {
            std::cerr << "Connection failed: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    // Close log file
    logFile.close();
    return 0;
}