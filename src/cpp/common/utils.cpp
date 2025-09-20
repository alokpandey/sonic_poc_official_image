/**
 * @file utils.cpp
 * @brief SONiC Common Utilities Implementation
 */

#include "utils.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace sonic {
namespace common {

std::string Utils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace common
} // namespace sonic
