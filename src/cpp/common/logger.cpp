/**
 * @file logger.cpp
 * @brief SONiC Common Logger Implementation
 */

#include "logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <mutex>

namespace sonic {
namespace common {

static std::mutex log_mutex;

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::string level_str;
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO:  level_str = "INFO";  break;
        case LogLevel::WARN:  level_str = "WARN";  break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }
    
    std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
              << "] [" << level_str << "] " << message << std::endl;
}

} // namespace common
} // namespace sonic
