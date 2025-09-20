/**
 * @file bsp_demo.cpp
 * @brief BSP Demo Application
 */

#include "../bsp/platform_health_monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace sonic::bsp;

int main() {
    std::cout << "SONiC BSP Demo Starting..." << std::endl;

    PlatformHealthMonitor monitor;

    if (!monitor.start()) {
        std::cerr << "Failed to start health monitor" << std::endl;
        return 1;
    }

    // Run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    monitor.stop();

    std::cout << "BSP Demo completed successfully" << std::endl;
    return 0;
}