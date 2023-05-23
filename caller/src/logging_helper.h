#ifndef LOGGING_HELPER_H
#define LOGGING_HELPER_H

#include <string>
#include <chrono>
#include <fstream>


uint64_t get_time() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

void write_log(std::string role, std::string content) {
    std::ofstream logFile("/logs/" + role + ".csv", std::ios_base::app);
    if (!logFile) {
        std::cerr << "Failed to open log!" << std::endl;
    }
    logFile << content << std::endl;
    logFile.close();
}
#endif
