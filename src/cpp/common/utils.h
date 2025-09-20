/**
 * @file utils.h
 * @brief SONiC Common Utilities Header
 */

#ifndef SONIC_COMMON_UTILS_H
#define SONIC_COMMON_UTILS_H

#include <string>

namespace sonic {
namespace common {

class Utils {
public:
    static std::string getCurrentTimestamp();
};

} // namespace common
} // namespace sonic

#endif // SONIC_COMMON_UTILS_H
