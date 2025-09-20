/**
 * @file main.cpp
 * @brief SONiC POC Main Application
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>

// SONiC component headers
#include "bsp/platform_health_monitor.h"
#include "sai/sai_vlan_manager.h"
#include "swss/orchagent.h"

using namespace sonic;

// Global flag for graceful shutdown
volatile sig_atomic_t g_shutdown = 0;

/**
 * @brief Signal handler for graceful shutdown
 */
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", initiating graceful shutdown..." << std::endl;
    g_shutdown = 1;
}

/**
 * @brief Print SONiC POC banner
 */
void printBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                    SONiC POC - C++ Implementation            ║
║              Software for Open Networking in the Cloud      ║
╠══════════════════════════════════════════════════════════════╣
║  Components:                                                 ║
║    • BSP (Board Support Package)                             ║
║    • SAI (Switch Abstraction Interface)                      ║
║    • SwSS (Switch State Service)                             ║
║    • Syncd (Synchronous Daemon)                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

/**
 * @brief Initialize and start BSP components
 */
std::unique_ptr<bsp::PlatformHealthMonitor> initializeBSP() {
    std::cout << "Initializing BSP components..." << std::endl;
    
    auto health_monitor = std::make_unique<bsp::PlatformHealthMonitor>();
    
    if (!health_monitor->start()) {
        std::cerr << "Failed to start platform health monitor" << std::endl;
        return nullptr;
    }
    
    std::cout << "BSP components initialized successfully" << std::endl;
    return health_monitor;
}

/**
 * @brief Initialize and start SAI components
 */
std::unique_ptr<sai::SAIVLANManager> initializeSAI() {
    std::cout << "Initializing SAI components..." << std::endl;
    
    auto vlan_manager = std::make_unique<sai::SAIVLANManager>();
    
    if (!vlan_manager->isInitialized()) {
        std::cerr << "Failed to initialize SAI VLAN manager" << std::endl;
        return nullptr;
    }
    
    // Create some demo VLANs
    vlan_manager->createVLAN(100, "Engineering");
    vlan_manager->createVLAN(200, "Sales");
    vlan_manager->addPortToVLAN(100, "Ethernet0", false);
    vlan_manager->addPortToVLAN(200, "Ethernet4", false);
    
    std::cout << "SAI components initialized successfully" << std::endl;
    return vlan_manager;
}

/**
 * @brief Initialize and start SwSS components
 */
std::unique_ptr<swss::OrchAgent> initializeSwSS() {
    std::cout << "Initializing SwSS components..." << std::endl;
    
    auto orch_agent = std::make_unique<swss::OrchAgent>();
    
    if (!orch_agent->start()) {
        std::cerr << "Failed to start orchestration agent" << std::endl;
        return nullptr;
    }
    
    std::cout << "SwSS components initialized successfully" << std::endl;
    return orch_agent;
}

/**
 * @brief Run system status monitoring
 */
void runStatusMonitoring(bsp::PlatformHealthMonitor* health_monitor,
                        sai::SAIVLANManager* vlan_manager) {
    
    std::cout << "\n=== System Status Monitoring ===" << std::endl;
    
    while (!g_shutdown) {
        // Display health status
        if (health_monitor && health_monitor->isRunning()) {
            auto health = health_monitor->getCurrentHealth();
            std::cout << "\n[HEALTH] " << health.timestamp 
                      << " CPU=" << health.cpu_temperature << "°C"
                      << " Power=" << health.power_consumption << "W"
                      << " Memory=" << health.memory_usage << "%" << std::endl;
            
            // Check for recent alerts
            auto alerts = health_monitor->getRecentAlerts(3);
            for (const auto& alert : alerts) {
                std::cout << "[ALERT] " << alert.message << std::endl;
            }
        }
        
        // Display VLAN status
        if (vlan_manager && vlan_manager->isInitialized()) {
            static int vlan_counter = 0;
            if (++vlan_counter % 10 == 0) { // Every 10 iterations
                std::cout << "\n[VLAN] Current VLANs:" << std::endl;
                auto vlans = vlan_manager->getAllVLANs();
                for (const auto& vlan : vlans) {
                    std::cout << "  VLAN " << vlan.vlan_id << " (" << vlan.name 
                              << ") - " << vlan.members.size() << " members" << std::endl;
                }
            }
        }
        
        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Print banner
    printBanner();
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize components
        std::cout << "Starting SONiC POC initialization..." << std::endl;
        
        // Initialize BSP
        auto health_monitor = initializeBSP();
        if (!health_monitor) {
            std::cerr << "Failed to initialize BSP components" << std::endl;
            return 1;
        }
        
        // Initialize SAI
        auto vlan_manager = initializeSAI();
        if (!vlan_manager) {
            std::cerr << "Failed to initialize SAI components" << std::endl;
            return 1;
        }
        
        // Initialize SwSS
        auto orch_agent = initializeSwSS();
        if (!orch_agent) {
            std::cerr << "Failed to initialize SwSS components" << std::endl;
            return 1;
        }
        
        std::cout << "\nSONiC POC initialization completed successfully!" << std::endl;
        std::cout << "System is now operational. Press Ctrl+C to shutdown gracefully." << std::endl;
        
        // Run main monitoring loop
        runStatusMonitoring(health_monitor.get(), vlan_manager.get());
        
        // Graceful shutdown
        std::cout << "\nInitiating graceful shutdown..." << std::endl;
        
        if (orch_agent) {
            orch_agent->stop();
        }
        
        if (health_monitor) {
            health_monitor->stop();
        }
        
        std::cout << "SONiC POC shutdown completed successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
    
    return 0;
}

/**
 * @brief Display help information
 */
void displayHelp() {
    std::cout << R"(
SONiC POC - C++ Implementation

Usage: sonic_poc [options]

Options:
  -h, --help          Show this help message
  -v, --version       Show version information
  -c, --config FILE   Use custom configuration file
  -d, --daemon        Run as daemon
  -l, --log-level     Set log level (debug, info, warn, error)

Examples:
  sonic_poc                    # Run with default configuration
  sonic_poc -c /etc/sonic.conf # Run with custom config
  sonic_poc -d                 # Run as daemon

For more information, visit: https://github.com/sonic-net/SONiC
)" << std::endl;
}
