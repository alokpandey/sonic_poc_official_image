#include "sonic_functional_tests.h"
#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>
#include <random>
#include <iomanip>
#include <chrono>
#include <sstream>

using namespace sonic::tests;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\nSONiC Functional Test Runner - Comprehensive HAL and SAI Testing\n"
              << "\nOptions:\n"
              << "  -h, --help              Show this help message\n"
              << "  -v, --verbose           Enable verbose output\n"
              << "  -q, --quiet             Quiet mode (minimal output)\n"
              << "  -s, --stop-on-failure   Stop on first test failure\n"
              << "  -t, --timeout SECONDS   Set test timeout (default: 30)\n"
              << "  -o, --output FILE       Save results to file\n"
              << "  --hal-only              Run only HAL tests\n"
              << "  --sai-only              Run only SAI tests\n"
              << "  --interrupt-only        Run only interrupt/cable event tests\n"
              << "  --integration-only      Run only integration tests\n"
              << "  --stress-tests          Run stress tests\n"
              << "  --quick                 Run quick test suite\n"
              << "\nTest Categories:\n"
              << "  HAL Tests:              Fan control, temperature monitoring, PSU, LED, interfaces\n"
              << "  SAI Tests:              VLAN management, port configuration, FDB, routing\n"
              << "  Interrupt Tests:        Cable insertion/removal, link flaps, SFP hot swap\n"
              << "  Integration Tests:      End-to-end scenarios, failure recovery\n"
              << "  Stress Tests:           Performance, scalability, resource limits\n"
              << "\nExamples:\n"
              << "  " << program_name << " --verbose                    # Run all tests with verbose output\n"
              << "  " << program_name << " --sai-only --output results.txt  # Run SAI tests, save to file\n"
              << "  " << program_name << " --quick --stop-on-failure    # Quick test with early exit\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Default configuration
    bool verbose = false;
    bool quiet = false;
    bool stop_on_failure = false;
    bool hal_only = false;
    bool sai_only = false;
    bool interrupt_only = false;
    bool integration_only = false;
    bool stress_tests = false;
    bool quick_mode = false;
    int timeout = 30;
    std::string output_file;
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"stop-on-failure", no_argument, 0, 's'},
        {"timeout", required_argument, 0, 't'},
        {"output", required_argument, 0, 'o'},
        {"hal-only", no_argument, 0, 1001},
        {"sai-only", no_argument, 0, 1002},
        {"interrupt-only", no_argument, 0, 1003},
        {"integration-only", no_argument, 0, 1004},
        {"stress-tests", no_argument, 0, 1005},
        {"quick", no_argument, 0, 1006},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "hvqst:o:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                verbose = true;
                break;
            case 'q':
                quiet = true;
                break;
            case 's':
                stop_on_failure = true;
                break;
            case 't':
                timeout = std::stoi(optarg);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 1001:
                hal_only = true;
                break;
            case 1002:
                sai_only = true;
                break;
            case 1003:
                interrupt_only = true;
                break;
            case 1004:
                integration_only = true;
                break;
            case 1005:
                stress_tests = true;
                break;
            case 1006:
                quick_mode = true;
                break;
            case '?':
                std::cerr << "Unknown option. Use --help for usage information." << std::endl;
                return 1;
            default:
                break;
        }
    }
    
    // Validate conflicting options
    if (quiet && verbose) {
        std::cerr << "Error: Cannot use both --quiet and --verbose options" << std::endl;
        return 1;
    }
    
    int exclusive_count = (hal_only ? 1 : 0) + (sai_only ? 1 : 0) +
                         (interrupt_only ? 1 : 0) + (integration_only ? 1 : 0) +
                         (stress_tests ? 1 : 0) + (quick_mode ? 1 : 0);
    if (exclusive_count > 1) {
        std::cerr << "Error: Cannot use multiple exclusive test mode options" << std::endl;
        return 1;
    }
    
    // Print banner
    if (!quiet) {
        std::cout << "\n"
                  << "╔══════════════════════════════════════════════════════════════╗\n"
                  << "║                 SONiC Functional Test Suite                 ║\n"
                  << "║              Hardware Abstraction Layer (HAL)               ║\n"
                  << "║            Switch Abstraction Interface (SAI)               ║\n"
                  << "║                    Integration Testing                      ║\n"
                  << "╚══════════════════════════════════════════════════════════════╝\n"
                  << std::endl;
        
        std::cout << "Configuration:\n"
                  << "  Verbose Mode: " << (verbose ? "Enabled" : "Disabled") << "\n"
                  << "  Stop on Failure: " << (stop_on_failure ? "Enabled" : "Disabled") << "\n"
                  << "  Timeout: " << timeout << " seconds\n";
        
        if (!output_file.empty()) {
            std::cout << "  Output File: " << output_file << "\n";
        }
        
        std::cout << std::endl;
    }
    
    // Initialize test framework
    SONiCFunctionalTests test_framework;
    test_framework.setVerboseMode(verbose && !quiet);
    test_framework.setStopOnFirstFailure(stop_on_failure);
    test_framework.setTimeout(timeout);
    
    if (!test_framework.initialize()) {
        std::cerr << "Failed to initialize SONiC Functional Test Framework" << std::endl;
        return 1;
    }
    
    bool overall_success = true;
    
    try {
        if (quick_mode) {
            // Quick test mode - run essential tests only
            if (!quiet) {
                std::cout << "Running Quick Test Suite...\n" << std::endl;
            }
            
            // Run a subset of critical tests
            auto hal_results = test_framework.runHALTests();
            auto sai_results = test_framework.runSAITests();
            
            overall_success = (hal_results.failed_tests == 0 && sai_results.failed_tests == 0);
            
        } else if (hal_only) {
            // HAL tests only
            if (!quiet) {
                std::cout << "Running HAL Tests Only...\n" << std::endl;
            }
            auto hal_results = test_framework.runHALTests();
            overall_success = (hal_results.failed_tests == 0);
            
        } else if (sai_only) {
            // SAI tests only
            if (!quiet) {
                std::cout << "Running SAI Tests Only...\n" << std::endl;
            }
            auto sai_results = test_framework.runSAITests();
            overall_success = (sai_results.failed_tests == 0);

        } else if (interrupt_only) {
            // Interrupt tests only
            if (!quiet) {
                std::cout << "Running Interrupt Tests Only...\n" << std::endl;
            }
            auto interrupt_results = test_framework.runInterruptTests();
            overall_success = (interrupt_results.failed_tests == 0);

        } else if (integration_only) {
            // Integration tests only
            if (!quiet) {
                std::cout << "Running Integration Tests Only...\n" << std::endl;
            }
            auto integration_results = test_framework.runIntegrationTests();
            overall_success = (integration_results.failed_tests == 0);
            
        } else if (stress_tests) {
            // Stress tests only
            if (!quiet) {
                std::cout << "Running Stress Tests...\n" << std::endl;
            }
            auto stress_results = test_framework.runStressTests();
            overall_success = (stress_results.failed_tests == 0);
            
        } else {
            // Run all tests
            if (!quiet) {
                std::cout << "Running Complete Test Suite...\n" << std::endl;
            }
            overall_success = test_framework.runAllTests();
        }
        
        // Save results to file if specified
        if (!output_file.empty()) {
            if (test_framework.saveResultsToFile(output_file)) {
                if (!quiet) {
                    std::cout << "\nTest results saved to: " << output_file << std::endl;
                }
            } else {
                std::cerr << "Failed to save results to file: " << output_file << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
        overall_success = false;
    }
    
    // Print final result
    if (!quiet) {
        std::cout << "\n"
                  << "╔══════════════════════════════════════════════════════════════╗\n"
                  << "║                      FINAL RESULT                           ║\n"
                  << "╠══════════════════════════════════════════════════════════════╣\n";
        
        if (overall_success) {
            std::cout << "║                    ✅ ALL TESTS PASSED                      ║\n";
        } else {
            std::cout << "║                    ❌ SOME TESTS FAILED                     ║\n";
        }
        
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n"
                  << std::endl;
    }
    
    // Cleanup
    test_framework.cleanup();
    
    return overall_success ? 0 : 1;
}

// Test Utilities Implementation
namespace sonic {
namespace tests {

std::string TestUtils::generateRandomMAC() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ss << ":";
        ss << std::setw(2) << dis(gen);
    }
    return ss.str();
}

std::string TestUtils::generateRandomIP() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254);
    
    return "192.168." + std::to_string(dis(gen)) + "." + std::to_string(dis(gen));
}

std::vector<std::string> TestUtils::getAvailablePorts(int count) {
    std::vector<std::string> ports;
    // Use actual SONiC port names (every 4th port starting from 0)
    for (int i = 0; i < count && i < 32; ++i) {
        ports.push_back("Ethernet" + std::to_string(i * 4));
    }
    return ports;
}

std::vector<uint16_t> TestUtils::generateVLANRange(uint16_t start, uint16_t count) {
    std::vector<uint16_t> vlans;
    for (uint16_t i = 0; i < count; ++i) {
        uint16_t vlan_id = start + i;
        if (vlan_id >= 1 && vlan_id <= 4094) {
            vlans.push_back(vlan_id);
        }
    }
    return vlans;
}

bool TestUtils::compareFloats(float a, float b, float tolerance) {
    return std::abs(a - b) <= tolerance;
}

std::string TestUtils::formatDuration(double milliseconds) {
    if (milliseconds < 1000) {
        return std::to_string(static_cast<int>(milliseconds)) + "ms";
    } else {
        return std::to_string(milliseconds / 1000.0) + "s";
    }
}

std::string TestUtils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

} // namespace tests
} // namespace sonic
