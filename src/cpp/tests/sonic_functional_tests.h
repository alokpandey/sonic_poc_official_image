#ifndef SONIC_FUNCTIONAL_TESTS_H
#define SONIC_FUNCTIONAL_TESTS_H

#include "../hal/sonic_hal_controller.h"
#include "../sai/sonic_sai_controller.h"
#include "../interrupts/sonic_interrupt_controller.h"
#include <string>
#include <vector>
#include <memory>

namespace sonic {
namespace tests {

// Test Result Structure
struct TestResult {
    std::string test_name;
    bool passed;
    std::string description;
    std::string error_message;
    double execution_time_ms;
    std::vector<std::string> details;
};

// Test Suite Results
struct TestSuiteResult {
    std::string suite_name;
    int total_tests;
    int passed_tests;
    int failed_tests;
    double total_execution_time_ms;
    std::vector<TestResult> test_results;
};

// Main Functional Test Class
class SONiCFunctionalTests {
public:
    SONiCFunctionalTests();
    ~SONiCFunctionalTests();

    // Initialize test environment
    bool initialize();
    void cleanup();

    // Run all functional tests
    bool runAllTests();
    
    // HAL Functional Tests
    TestSuiteResult runHALTests();
    TestResult testFanSpeedControl();
    TestResult testTemperatureMonitoring();
    TestResult testPowerSupplyControl();
    TestResult testLEDControl();
    TestResult testInterfaceHALControl();
    TestResult testSystemInformation();

    // SAI Functional Tests
    TestSuiteResult runSAITests();
    TestResult testVLANCreationDeletion();
    TestResult testVLANMemberManagement();
    TestResult testPortConfiguration();
    TestResult testPortStatusControl();
    TestResult testMultipleVLANOperations();
    TestResult testVLANPortInteraction();

    // Advanced SAI Tests
    TestResult testFDBManagement();
    TestResult testRoutingOperations();
    TestResult testACLOperations();
    TestResult testLAGOperations();

    // Interrupt and Cable Event Tests
    TestSuiteResult runInterruptTests();
    TestResult testCableInsertionRemoval();
    TestResult testLinkFlapDetection();
    TestResult testSFPHotSwap();
    TestResult testMultiPortCableEvents();
    TestResult testSONiCCLIResponseToEvents();
    TestResult testEventTimingValidation();
    TestResult testInterruptHandlerRegistration();

    // Integration Tests
    TestSuiteResult runIntegrationTests();
    TestResult testHALSAIIntegration();
    TestResult testCompleteNetworkSetup();
    TestResult testFailureRecovery();
    TestResult testPerformanceBaseline();

    // Stress Tests
    TestSuiteResult runStressTests();
    TestResult testMassVLANCreation();
    TestResult testRapidPortToggling();
    TestResult testConcurrentOperations();
    TestResult testResourceLimits();

    // Validation Tests
    TestSuiteResult runValidationTests();
    TestResult testDataConsistency();
    TestResult testConfigPersistence();
    TestResult testErrorHandling();
    TestResult testBoundaryConditions();

    // Utility Functions
    void printTestResults(const TestSuiteResult& suite_result);
    void printSummary();
    bool saveResultsToFile(const std::string& filename);
    
    // Test Configuration
    void setVerboseMode(bool verbose);
    void setStopOnFirstFailure(bool stop);
    void setTimeout(int timeout_seconds);

private:
    std::unique_ptr<hal::SONiCHALController> m_hal_controller;
    std::unique_ptr<sai::SONiCSAIController> m_sai_controller;
    std::unique_ptr<interrupts::SONiCInterruptController> m_interrupt_controller;
    
    bool m_initialized;
    bool m_verbose_mode;
    bool m_stop_on_failure;
    int m_timeout_seconds;
    
    // Test execution helpers
    TestResult executeTest(const std::string& test_name, 
                          const std::string& description,
                          std::function<bool()> test_function);
    
    bool waitForCondition(std::function<bool()> condition, int timeout_ms = 5000);
    void logTestStep(const std::string& step);
    void logTestError(const std::string& error);
    void logTestWarning(const std::string& warning);
    void logTestInfo(const std::string& info);
    
    // Test data management
    void setupTestEnvironment();
    void cleanupTestEnvironment();
    bool verifyInitialState();
    
    // Test validation helpers
    bool validateVLANExists(uint16_t vlan_id);
    bool validatePortInVLAN(const std::string& port_name, uint16_t vlan_id);
    bool validatePortStatus(const std::string& port_name, const std::string& expected_status);
    bool validateFanSpeed(int fan_id, int expected_speed_range_percent);
    bool validateTemperatureReading(int sensor_id, float min_temp, float max_temp);
    
    // Performance measurement
    std::chrono::high_resolution_clock::time_point m_test_start_time;
    void startTimer();
    double getElapsedTimeMs();
    
    // Test state tracking
    std::vector<uint16_t> m_created_vlans;
    std::vector<std::string> m_modified_ports;
    std::vector<std::pair<uint16_t, std::string>> m_vlan_port_associations;
    
    // Test statistics
    int m_total_tests_run;
    int m_total_tests_passed;
    int m_total_tests_failed;
    double m_total_execution_time_ms;
    
    // Test results storage
    std::vector<TestSuiteResult> m_all_suite_results;
};

// Specific Test Scenarios
class SONiCTestScenarios {
public:
    // Real-world network scenarios
    static TestResult testEnterpriseNetworkSetup(SONiCFunctionalTests* test_framework);
    static TestResult testDataCenterTopology(SONiCFunctionalTests* test_framework);
    static TestResult testServiceProviderScenario(SONiCFunctionalTests* test_framework);
    
    // Hardware failure simulation
    static TestResult testFanFailureScenario(SONiCFunctionalTests* test_framework);
    static TestResult testPowerSupplyFailure(SONiCFunctionalTests* test_framework);
    static TestResult testPortFailureRecovery(SONiCFunctionalTests* test_framework);
    
    // Performance scenarios
    static TestResult testHighThroughputScenario(SONiCFunctionalTests* test_framework);
    static TestResult testLowLatencyScenario(SONiCFunctionalTests* test_framework);
    static TestResult testScalabilityScenario(SONiCFunctionalTests* test_framework);
};

// Test Utilities
class TestUtils {
public:
    static std::string generateRandomMAC();
    static std::string generateRandomIP();
    static std::vector<std::string> getAvailablePorts(int count = 4);
    static std::vector<uint16_t> generateVLANRange(uint16_t start, uint16_t count);
    static bool compareFloats(float a, float b, float tolerance = 0.1f);
    static std::string formatDuration(double milliseconds);
    static std::string getCurrentTimestamp();
};

} // namespace tests
} // namespace sonic

#endif // SONIC_FUNCTIONAL_TESTS_H
