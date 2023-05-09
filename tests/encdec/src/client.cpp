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
#include <boost/asio.hpp>

int length

int main(int argc, char **argv) {
    std::cout << "Starting PIRATES client ..." << std::endl;
    int c;
    while ((c = getopt(argc, argv, "l:")) != -1) {
        switch(c) {
            case 'l':
                length = std::stoi(optarg);
                break;
            default:
                abort();
        }
    }
    // Create a file for logging
    std::ofstream logFile("encdec_log.txt");
    if (!logFile) {
        std::cerr << "Failed to open log file!" << std::endl;
        return 1;
    }
    //LPCNetFreeDV* sample = lpcnet_freedv_create(1);
    //////////////////////////////////////////// LPCNET ENCODING ////////////////////////////////////////////
    std::cout << "Encoding voice snippet ..." << std::endl;
    // Input and output files for the encoded audio
    FILE *fin_enc, *fout_enc, *fin_dec, *fout_dec;
    fin_enc = fopen("/home/app/LPCNet/wav/birch.wav", "rb");
    fout_enc = fopen("/home/app/encoded_audio", "wb");
    // direct_split = 0
    LPCNetFreeDV *lf = lpcnet_freedv_create(0);
    // Taken from lpcnet_enc.c
    char frame[lpcnet_bits_per_frame(lf)];
    int f=0;
    int bits_written=0;
    short pcm[lpcnet_samples_per_frame(lf)];
    // Read from wav file and encode each read frame
    // We will run this in its own thread
    // To test, we only encode 1 frame
    auto start = std::chrono::high_resolution_clock::now();
    while (f < 1) {      
        int nread = fread(pcm, sizeof(short), lpcnet_samples_per_frame(lf), fin_enc);
        if (nread != lpcnet_samples_per_frame(lf)) break;

        lpcnet_enc(lf, pcm, frame);
        bits_written += fwrite(frame, sizeof(char), lpcnet_bits_per_frame(lf), fout_enc);

        fflush(stdin);
        fflush(stdout);
        f++;
    }
    // Close files for encoding
    fclose(fin_enc);
    fclose(fout_enc);
    auto end = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    //std::cout << "Needed time for LPCNet encoding: " << duration.count() << "ns\n";
    //std::cout << "Needed time for LPCNet encoding: " << duration.count() << "ms\n";
    
    std::cout << "Needed time for LPCNet encoding: " << duration.count() << "\u03bcs\n";
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
    std::ifstream in("/home/app/encoded_audio", std::ios::binary);
    std::ofstream out("/home/app/encrypted_audio", std::ios::binary);
    // Get file size
    in.seekg(0, std::ios::end);
    const auto file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    unsigned char buffer[16];
    unsigned char encrypted_buffer[16];
    start = std::chrono::high_resolution_clock::now();
    AES_cbc_encrypt(buffer, encrypted_buffer, 16, &aes_key, iv, AES_ENCRYPT);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Needed time for AES encryption: " << duration.count() << "\u03bcs\n";
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
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

    // Close input and output files
    in.close();
    out.close();
    //////////////////////////////////////////// SENDING VOICE SNIPPETS /////////////////////////////////////
    
    std::ifstream ifs("/home/app/encrypted_audio", std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open encrypted audio." << std::endl;
        return 1;
    }
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    bool connected = false;
    while (!connected) {
        try {
            tcp::resolver::results_type endpoints = resolver.resolve(RELAY_IP, "8080");
            tcp::socket socket(io_context);
            boost::asio::connect(socket, endpoints);
            boost::system::error_code error;
            while (ifs.good()) {
                char buf[1024];
                ifs.read(buf, sizeof(buf));
                auto bytes_read = ifs.gcount();
                if (bytes_read > 0) {
                    boost::asio::write(socket, boost::asio::buffer(buf, bytes_read), error);
                    if (error) {
                        std::cerr << "Error sending file: " << error.message() << std::endl;
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
    //////////////////////////////////////////// LPCNET DECODING ////////////////////////////////////////////
    std::cout << "Decoding voice snippet ..." << std::endl;
    // Open files
    fin_dec = fopen("/home/app/encoded_audio", "rb");
    fout_dec = fopen("/home/app/decoded_audio", "wb");
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
}
