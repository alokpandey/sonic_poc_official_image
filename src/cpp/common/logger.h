/**
 * @file logger.h
 * @brief SONiC Common Logger Header
 */

#ifndef SONIC_COMMON_LOGGER_H
#define SONIC_COMMON_LOGGER_H

#include <string>

namespace sonic {
namespace common {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void log(LogLevel level, const std::string& message);
};

} // namespace common
} // namespace sonic

#endif // SONIC_COMMON_LOGGER_H
