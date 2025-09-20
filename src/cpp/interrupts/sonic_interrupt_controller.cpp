#include "sonic_interrupt_controller.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <random>
#include <regex>
#include <iomanip>
#include <mutex>
#include <functional>

namespace sonic {
namespace interrupts {

SONiCInterruptController::SONiCInterruptController()
    : m_initialized(false), m_monitoring(false), m_sonic_container_name("sonic-vs-official"), m_verbose_debug(true) {
}

SONiCInterruptController::~SONiCInterruptController() {
    cleanup();
}

bool SONiCInterruptController::initialize() {
    std::cout << "[INTERRUPT] Initializing SONiC Interrupt Controller..." << std::endl;
    
    // Test connection to SONiC container
    std::string output;
    if (!executeSONiCCommand("echo 'INTERRUPT_TEST'", output)) {
        std::cerr << "[INTERRUPT] Failed to connect to SONiC container" << std::endl;
        return false;
    }
    
    // Initialize port states
    if (!refreshPortStatusFromSONiC()) {
        std::cerr << "[INTERRUPT] Failed to initialize port states" << std::endl;
        return false;
    }
    
    m_last_poll_time = std::chrono::system_clock::now();
    m_initialized = true;
    
    std::cout << "[INTERRUPT] SONiC Interrupt Controller initialized successfully" << std::endl;
    std::cout << "[INTERRUPT] Monitoring " << m_port_states.size() << " ports" << std::endl;
    
    return true;
}

void SONiCInterruptController::cleanup() {
    if (m_initialized) {
        std::cout << "[INTERRUPT] Cleaning up SONiC Interrupt Controller..." << std::endl;
        stopEventMonitoring();
        m_initialized = false;
    }
}

bool SONiCInterruptController::startEventMonitoring() {
    if (m_monitoring.load()) {
        std::cout << "[INTERRUPT] Event monitoring already started" << std::endl;
        return true;
    }

    m_monitoring.store(true);
    m_monitor_thread = std::make_unique<std::thread>(&SONiCInterruptController::monitoringLoop, this);

    std::cout << "[INTERRUPT] Event monitoring started" << std::endl;
    return true;
}

bool SONiCInterruptController::stopEventMonitoring() {
    if (!m_monitoring.load()) {
        return true; // Already stopped
    }

    std::cout << "[INTERRUPT] Stopping event monitoring..." << std::endl;
    m_monitoring.store(false);

    if (m_monitor_thread && m_monitor_thread->joinable()) {
        m_monitor_thread->join();
    }

    std::cout << "[INTERRUPT] Event monitoring stopped" << std::endl;
    return true;
}

bool SONiCInterruptController::isMonitoring() const {
    return m_monitoring.load();
}

void SONiCInterruptController::monitoringLoop() {
    std::cout << "[INTERRUPT] Monitoring loop started" << std::endl;

    while (m_monitoring.load()) {
        // Poll for port state changes
        detectPortChanges();

        // Sleep for poll interval
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }

    std::cout << "[INTERRUPT] Monitoring loop stopped" << std::endl;
}

bool SONiCInterruptController::detectPortChanges() {
    // This would normally detect real hardware changes
    // For now, it's a placeholder that could be extended
    return true;
}

bool SONiCInterruptController::executeSONiCCommand(const std::string& command, std::string& output) {
    // Use a simpler approach without bash -c to avoid escaping issues
    std::string full_command = "docker exec " + m_sonic_container_name + " " + command;

    // Debug output
    if (m_verbose_debug) {
        std::cout << "[INTERRUPT] Executing: " << full_command << std::endl;
    }

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        std::cerr << "[INTERRUPT] Failed to execute command: " << full_command << std::endl;
        return false;
    }

    char buffer[256];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int result = pclose(pipe);
    if (result != 0 && m_verbose_debug) {
        std::cerr << "[INTERRUPT] Command failed with exit code: " << result << std::endl;
        std::cerr << "[INTERRUPT] Output: " << output << std::endl;
    }

    return (result == 0);
}

bool SONiCInterruptController::executeRedisCommand(const std::string& command, int db_id, std::string& output) {
    std::stringstream ss;
    ss << "redis-cli -n " << db_id << " " << command;
    return executeSONiCCommand(ss.str(), output);
}

bool SONiCInterruptController::setRedisHashField(const std::string& key, const std::string& field,
                                                const std::string& value, int db_id) {
    std::string output;
    std::stringstream ss;
    ss << "HSET \"" << key << "\" \"" << field << "\" \"" << value << "\"";
    return executeRedisCommand(ss.str(), db_id, output);
}

std::string SONiCInterruptController::getRedisHashField(const std::string& key, const std::string& field, int db_id) {
    std::string output;
    std::stringstream ss;
    ss << "HGET \"" << key << "\" \"" << field << "\"";
    if (executeRedisCommand(ss.str(), db_id, output)) {
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    return "";
}

// Cable Event Simulation Implementation
bool SONiCInterruptController::simulateCableInsertion(const std::string& port_name) {
    std::cout << "[INTERRUPT] Simulating cable insertion on " << port_name << std::endl;
    
    if (!validatePortName(port_name)) {
        std::cerr << "[INTERRUPT] Invalid port name: " << port_name << std::endl;
        return false;
    }
    
    // Get current state
    std::lock_guard<std::mutex> lock(m_state_mutex);
    LinkState current_state = getPortLinkStateUnsafe(port_name);
    LinkStatus old_status = current_state.oper_status;
    
    // Simulate cable insertion by:
    // 1. Setting transceiver present
    // 2. Updating link status to UP
    // 3. Updating SONiC databases
    
    // Update APPL_DB with link up
    bool result = setRedisHashField("PORT_TABLE:" + port_name, "oper_status", "up", 0);
    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update APPL_DB for " << port_name << std::endl;
        return false;
    }
    
    // Update STATE_DB with transceiver info
    result = setRedisHashField("TRANSCEIVER_INFO|" + port_name, "present", "true", 6);
    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update transceiver info for " << port_name << std::endl;
        return false;
    }
    
    // Simulate some delay for link negotiation (reduced)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Update internal state
    current_state.oper_status = LinkStatus::UP;
    current_state.last_change = std::chrono::system_clock::now();
    current_state.link_up_count++;
    m_port_states[port_name] = current_state;
    
    // Create and trigger event
    PortEvent event;
    event.port_name = port_name;
    event.event_type = CableEvent::CABLE_INSERTED;
    event.old_status = old_status;
    event.new_status = LinkStatus::UP;
    event.speed_mbps = current_state.speed_mbps;
    event.duplex = current_state.duplex;
    event.timestamp = std::chrono::system_clock::now();
    event.additional_info = "Cable insertion simulated";
    
    triggerEvent(event);
    
    std::cout << "[INTERRUPT] Cable insertion simulated successfully on " << port_name << std::endl;
    return true;
}

bool SONiCInterruptController::simulateCableRemoval(const std::string& port_name) {
    std::cout << "[INTERRUPT] Simulating cable removal on " << port_name << std::endl;
    
    if (!validatePortName(port_name)) {
        std::cerr << "[INTERRUPT] Invalid port name: " << port_name << std::endl;
        return false;
    }
    
    // Get current state
    std::lock_guard<std::mutex> lock(m_state_mutex);
    LinkState current_state = getPortLinkStateUnsafe(port_name);
    LinkStatus old_status = current_state.oper_status;
    
    // Simulate cable removal by:
    // 1. Setting transceiver not present
    // 2. Updating link status to DOWN
    // 3. Updating SONiC databases
    
    // Update APPL_DB with link down
    bool result = setRedisHashField("PORT_TABLE:" + port_name, "oper_status", "down", 0);
    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update APPL_DB for " << port_name << std::endl;
        return false;
    }
    
    // Update STATE_DB with transceiver removal
    result = setRedisHashField("TRANSCEIVER_INFO|" + port_name, "present", "false", 6);
    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update transceiver info for " << port_name << std::endl;
        return false;
    }
    
    // Update internal state
    current_state.oper_status = LinkStatus::DOWN;
    current_state.last_change = std::chrono::system_clock::now();
    current_state.link_down_count++;
    m_port_states[port_name] = current_state;
    
    // Create and trigger event
    PortEvent event;
    event.port_name = port_name;
    event.event_type = CableEvent::CABLE_REMOVED;
    event.old_status = old_status;
    event.new_status = LinkStatus::DOWN;
    event.speed_mbps = current_state.speed_mbps;
    event.duplex = current_state.duplex;
    event.timestamp = std::chrono::system_clock::now();
    event.additional_info = "Cable removal simulated";
    
    triggerEvent(event);
    
    std::cout << "[INTERRUPT] Cable removal simulated successfully on " << port_name << std::endl;
    return true;
}

bool SONiCInterruptController::simulateLinkFlap(const std::string& port_name, int flap_count) {
    std::cout << "[INTERRUPT] Simulating link flap on " << port_name 
              << " (count: " << flap_count << ")" << std::endl;
    
    for (int i = 0; i < flap_count; i++) {
        std::cout << "[INTERRUPT] Flap " << (i + 1) << "/" << flap_count << std::endl;
        
        // Link down
        if (!simulateCableRemoval(port_name)) {
            std::cerr << "[INTERRUPT] Failed to simulate link down in flap " << (i + 1) << std::endl;
            return false;
        }
        
        // Wait a bit (reduced)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Link up
        if (!simulateCableInsertion(port_name)) {
            std::cerr << "[INTERRUPT] Failed to simulate link up in flap " << (i + 1) << std::endl;
            return false;
        }
        
        // Wait between flaps (reduced)
        if (i < flap_count - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "[INTERRUPT] Link flap simulation completed on " << port_name << std::endl;
    return true;
}

bool SONiCInterruptController::simulateSFPInsertion(const std::string& port_name, const SFPInfo& sfp_info) {
    std::cout << "[INTERRUPT] Simulating SFP insertion on " << port_name << std::endl;

    if (!validatePortName(port_name)) {
        std::cerr << "[INTERRUPT] Invalid port name: " << port_name << std::endl;
        return false;
    }

    // Update STATE_DB with SFP information
    std::string sfp_key = "TRANSCEIVER_INFO|" + port_name;
    bool result = setRedisHashField(sfp_key, "present", "true", 6);
    result &= setRedisHashField(sfp_key, "vendor_name", sfp_info.vendor_name, 6);
    result &= setRedisHashField(sfp_key, "part_number", sfp_info.part_number, 6);
    result &= setRedisHashField(sfp_key, "serial_number", sfp_info.serial_number, 6);

    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update SFP info for " << port_name << std::endl;
        return false;
    }

    // Update internal cache
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_sfp_info[port_name] = sfp_info;

    // Create and trigger event
    PortEvent event;
    event.port_name = port_name;
    event.event_type = CableEvent::SFP_INSERTED;
    event.old_status = LinkStatus::DOWN;
    event.new_status = LinkStatus::UP;
    event.timestamp = std::chrono::system_clock::now();
    event.additional_info = "SFP insertion simulated";

    triggerEvent(event);

    std::cout << "[INTERRUPT] SFP insertion simulated successfully on " << port_name << std::endl;
    return true;
}

bool SONiCInterruptController::simulateSFPRemoval(const std::string& port_name) {
    std::cout << "[INTERRUPT] Simulating SFP removal on " << port_name << std::endl;

    if (!validatePortName(port_name)) {
        std::cerr << "[INTERRUPT] Invalid port name: " << port_name << std::endl;
        return false;
    }

    // Update STATE_DB to remove SFP
    std::string sfp_key = "TRANSCEIVER_INFO|" + port_name;
    bool result = setRedisHashField(sfp_key, "present", "false", 6);

    if (!result) {
        std::cerr << "[INTERRUPT] Failed to update SFP removal for " << port_name << std::endl;
        return false;
    }

    // Update internal cache
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (m_sfp_info.find(port_name) != m_sfp_info.end()) {
        m_sfp_info[port_name].is_present = false;
    }

    // Create and trigger event
    PortEvent event;
    event.port_name = port_name;
    event.event_type = CableEvent::SFP_REMOVED;
    event.old_status = LinkStatus::UP;
    event.new_status = LinkStatus::DOWN;
    event.timestamp = std::chrono::system_clock::now();
    event.additional_info = "SFP removal simulated";

    triggerEvent(event);

    std::cout << "[INTERRUPT] SFP removal simulated successfully on " << port_name << std::endl;
    return true;
}

// Event Handler Registration
void SONiCInterruptController::registerEventHandler(CableEvent event_type, InterruptHandler handler) {
    std::lock_guard<std::mutex> lock(m_handler_mutex);
    m_event_handlers[event_type].push_back(handler);
    std::cout << "[INTERRUPT] Registered handler for event: " << cableEventToString(event_type) << std::endl;
}

void SONiCInterruptController::registerGlobalEventHandler(InterruptHandler handler) {
    std::lock_guard<std::mutex> lock(m_handler_mutex);
    m_global_handlers.push_back(handler);
    std::cout << "[INTERRUPT] Registered global event handler" << std::endl;
}

void SONiCInterruptController::triggerEvent(const PortEvent& event) {
    std::lock_guard<std::mutex> event_lock(m_event_mutex);
    
    // Add to event history
    m_event_history.push_back(event);
    updateEventStatistics(event.event_type);
    logEvent(event);
    
    // Trigger specific event handlers
    std::lock_guard<std::mutex> handler_lock(m_handler_mutex);
    auto it = m_event_handlers.find(event.event_type);
    if (it != m_event_handlers.end()) {
        for (const auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                std::cerr << "[INTERRUPT] Event handler exception: " << e.what() << std::endl;
            }
        }
    }
    
    // Trigger global handlers
    for (const auto& handler : m_global_handlers) {
        try {
            handler(event);
        } catch (const std::exception& e) {
            std::cerr << "[INTERRUPT] Global event handler exception: " << e.what() << std::endl;
        }
    }
}

// Port Status Queries
LinkState SONiCInterruptController::getPortLinkState(const std::string& port_name) {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    return getPortLinkStateUnsafe(port_name);
}

LinkState SONiCInterruptController::getPortLinkStateUnsafe(const std::string& port_name) {
    // This method assumes the mutex is already locked by the caller
    auto it = m_port_states.find(port_name);
    if (it != m_port_states.end()) {
        return it->second;
    }

    // Return default state if not found
    LinkState default_state;
    default_state.port_name = port_name;
    default_state.admin_status = LinkStatus::UNKNOWN;
    default_state.oper_status = LinkStatus::UNKNOWN;
    default_state.speed_mbps = 0;
    default_state.duplex = "unknown";
    default_state.auto_neg = false;
    default_state.mtu = 1500;
    default_state.mac_address = "00:00:00:00:00:00";
    default_state.last_change = std::chrono::system_clock::now();
    default_state.link_up_count = 0;
    default_state.link_down_count = 0;

    return default_state;
}

std::vector<LinkState> SONiCInterruptController::getAllPortStates() {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    std::vector<LinkState> states;
    for (const auto& pair : m_port_states) {
        states.push_back(pair.second);
    }
    return states;
}

SFPInfo SONiCInterruptController::getSFPInfo(const std::string& port_name) {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_sfp_info.find(port_name);
    if (it != m_sfp_info.end()) {
        return it->second;
    }

    // Return default SFP info if not found
    SFPInfo default_sfp;
    default_sfp.port_name = port_name;
    default_sfp.is_present = false;
    default_sfp.vendor_name = "";
    default_sfp.part_number = "";
    default_sfp.serial_number = "";
    default_sfp.connector_type = "";
    default_sfp.cable_length = "";
    default_sfp.status = "not_present";

    return default_sfp;
}

// SONiC CLI Integration
bool SONiCInterruptController::refreshPortStatusFromSONiC() {
    std::cout << "[INTERRUPT] Refreshing port status from SONiC..." << std::endl;
    
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_port_states.clear();
    
    // Get port list from CONFIG_DB
    std::string output;
    if (!executeRedisCommand("KEYS \"PORT|*\"", 4, output)) {
        std::cerr << "[INTERRUPT] Failed to get port list from CONFIG_DB" << std::endl;
        return false;
    }
    
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("PORT|") == 0) {
            std::string port_name = line.substr(5); // Remove "PORT|" prefix
            
            LinkState state;
            state.port_name = port_name;
            
            // Get admin status from CONFIG_DB
            std::string admin_status = getRedisHashField("PORT|" + port_name, "admin_status", 4);
            state.admin_status = parseSONiCLinkStatus(admin_status);
            
            // Get operational status from APPL_DB
            std::string oper_status = getRedisHashField("PORT_TABLE:" + port_name, "oper_status", 0);
            state.oper_status = parseSONiCLinkStatus(oper_status);
            
            // Get other port information
            std::string speed_str = getRedisHashField("PORT|" + port_name, "speed", 4);
            state.speed_mbps = speed_str.empty() ? 100000 : std::stoul(speed_str);
            
            std::string mtu_str = getRedisHashField("PORT|" + port_name, "mtu", 4);
            state.mtu = mtu_str.empty() ? 9100 : std::stoul(mtu_str);
            
            state.duplex = "full";
            state.auto_neg = true;
            state.mac_address = "02:42:ac:19:00:0a"; // Default MAC
            state.last_change = std::chrono::system_clock::now();
            state.link_up_count = 0;
            state.link_down_count = 0;
            
            m_port_states[port_name] = state;
        }
    }
    
    std::cout << "[INTERRUPT] Refreshed " << m_port_states.size() << " port states" << std::endl;
    return true;
}

bool SONiCInterruptController::verifySONiCPortStatus(const std::string& port_name, LinkStatus expected_status) {
    std::cout << "[INTERRUPT] Verifying SONiC port status for " << port_name
              << " (expected: " << linkStatusToString(expected_status) << ")" << std::endl;

    // Get status from Redis APPL_DB instead of CLI (faster and more reliable)
    std::string oper_status = getRedisHashField("PORT_TABLE:" + port_name, "oper_status", 0);

    LinkStatus actual_status = parseSONiCLinkStatus(oper_status);
    bool status_matches = (actual_status == expected_status);

    std::cout << "[INTERRUPT] SONiC status verification: "
              << (status_matches ? "PASSED" : "FAILED") << std::endl;
    std::cout << "[INTERRUPT] Expected: " << linkStatusToString(expected_status)
              << ", Actual: " << linkStatusToString(actual_status) << " (from Redis: '" << oper_status << "')" << std::endl;

    return status_matches;
}

std::string SONiCInterruptController::getSONiCInterfaceStatus(const std::string& port_name) {
    // Use Redis instead of CLI for faster response
    std::string admin_status = getRedisHashField("PORT|" + port_name, "admin_status", 4);
    std::string oper_status = getRedisHashField("PORT_TABLE:" + port_name, "oper_status", 0);

    std::stringstream ss;
    ss << "Interface " << port_name << ":\n";
    ss << "  Admin Status: " << admin_status << "\n";
    ss << "  Oper Status: " << oper_status << "\n";

    return ss.str();
}

std::string SONiCInterruptController::getSONiCTransceiverInfo(const std::string& port_name) {
    // Use Redis instead of CLI for faster response
    std::string present = getRedisHashField("TRANSCEIVER_INFO|" + port_name, "present", 6);
    std::string vendor = getRedisHashField("TRANSCEIVER_INFO|" + port_name, "vendor_name", 6);

    std::stringstream ss;
    ss << "Transceiver " << port_name << ":\n";
    ss << "  Present: " << present << "\n";
    ss << "  Vendor: " << vendor << "\n";

    return ss.str();
}

// Test Functions Implementation
bool SONiCInterruptController::runInterruptTests() {
    std::cout << "\n=== Running SONiC Interrupt Tests ===" << std::endl;

    bool all_passed = true;

    // Test 1: Cable Insertion/Removal
    if (!testCableInsertionRemoval()) {
        std::cerr << "[INTERRUPT] Cable insertion/removal test FAILED" << std::endl;
        all_passed = false;
    } else {
        std::cout << "[INTERRUPT] Cable insertion/removal test PASSED" << std::endl;
    }

    // Test 2: Link Flap Detection
    if (!testLinkFlapDetection()) {
        std::cerr << "[INTERRUPT] Link flap detection test FAILED" << std::endl;
        all_passed = false;
    } else {
        std::cout << "[INTERRUPT] Link flap detection test PASSED" << std::endl;
    }

    // Test 3: SONiC CLI Response
    if (!testSONiCCLIResponse()) {
        std::cerr << "[INTERRUPT] SONiC CLI response test FAILED" << std::endl;
        all_passed = false;
    } else {
        std::cout << "[INTERRUPT] SONiC CLI response test PASSED" << std::endl;
    }

    // Test 4: Multi-port Events
    if (!testMultiPortEvents()) {
        std::cerr << "[INTERRUPT] Multi-port events test FAILED" << std::endl;
        all_passed = false;
    } else {
        std::cout << "[INTERRUPT] Multi-port events test PASSED" << std::endl;
    }

    // Test 5: Event Timing
    if (!testEventTiming()) {
        std::cerr << "[INTERRUPT] Event timing test FAILED" << std::endl;
        all_passed = false;
    } else {
        std::cout << "[INTERRUPT] Event timing test PASSED" << std::endl;
    }

    return all_passed;
}

bool SONiCInterruptController::testCableInsertionRemoval() {
    std::cout << "\n[INTERRUPT] Testing Cable Insertion/Removal..." << std::endl;

    // Get a test port
    auto test_ports = InterruptUtils::getTestPorts(1);
    if (test_ports.empty()) {
        std::cerr << "[INTERRUPT] No test ports available" << std::endl;
        return false;
    }

    std::string test_port = test_ports[0];
    std::cout << "[INTERRUPT] Using test port: " << test_port << std::endl;

    // Set up event tracking
    bool cable_inserted_detected = false;
    bool cable_removed_detected = false;

    registerEventHandler(CableEvent::CABLE_INSERTED,
        [&cable_inserted_detected, test_port](const PortEvent& event) {
            if (event.port_name == test_port && event.event_type == CableEvent::CABLE_INSERTED) {
                std::cout << "[INTERRUPT] Cable insertion event detected for " << test_port << std::endl;
                cable_inserted_detected = true;
            }
        });

    registerEventHandler(CableEvent::CABLE_REMOVED,
        [&cable_removed_detected, test_port](const PortEvent& event) {
            if (event.port_name == test_port && event.event_type == CableEvent::CABLE_REMOVED) {
                std::cout << "[INTERRUPT] Cable removal event detected for " << test_port << std::endl;
                cable_removed_detected = true;
            }
        });

    // Test cable insertion
    std::cout << "[INTERRUPT] Step 1: Simulating cable insertion..." << std::endl;
    if (!simulateCableInsertion(test_port)) {
        std::cerr << "[INTERRUPT] Failed to simulate cable insertion" << std::endl;
        return false;
    }

    // Wait for event processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Verify SONiC CLI shows link up
    std::cout << "[INTERRUPT] Step 2: Verifying SONiC CLI shows link up..." << std::endl;
    if (!verifySONiCPortStatus(test_port, LinkStatus::UP)) {
        std::cerr << "[INTERRUPT] SONiC CLI does not show link up" << std::endl;
        return false;
    }

    // Test cable removal
    std::cout << "[INTERRUPT] Step 3: Simulating cable removal..." << std::endl;
    if (!simulateCableRemoval(test_port)) {
        std::cerr << "[INTERRUPT] Failed to simulate cable removal" << std::endl;
        return false;
    }

    // Wait for event processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Verify SONiC CLI shows link down
    std::cout << "[INTERRUPT] Step 4: Verifying SONiC CLI shows link down..." << std::endl;
    if (!verifySONiCPortStatus(test_port, LinkStatus::DOWN)) {
        std::cerr << "[INTERRUPT] SONiC CLI does not show link down" << std::endl;
        return false;
    }

    // Check event detection
    if (!cable_inserted_detected) {
        std::cerr << "[INTERRUPT] Cable insertion event was not detected" << std::endl;
        return false;
    }

    if (!cable_removed_detected) {
        std::cerr << "[INTERRUPT] Cable removal event was not detected" << std::endl;
        return false;
    }

    std::cout << "[INTERRUPT] Cable insertion/removal test completed successfully" << std::endl;
    return true;
}

bool SONiCInterruptController::testLinkFlapDetection() {
    std::cout << "\n[INTERRUPT] Testing Link Flap Detection..." << std::endl;

    // Get a test port
    auto test_ports = InterruptUtils::getTestPorts(1);
    if (test_ports.empty()) {
        std::cerr << "[INTERRUPT] No test ports available" << std::endl;
        return false;
    }

    std::string test_port = test_ports[0];
    std::cout << "[INTERRUPT] Using test port: " << test_port << std::endl;

    // Track flap events
    int flap_count = 0;
    int expected_flaps = 3;

    registerGlobalEventHandler([&flap_count, test_port, this](const PortEvent& event) {
        if (event.port_name == test_port &&
            (event.event_type == CableEvent::CABLE_INSERTED ||
             event.event_type == CableEvent::CABLE_REMOVED)) {
            flap_count++;
            std::cout << "[INTERRUPT] Flap event " << flap_count << " detected: "
                      << this->cableEventToString(event.event_type) << std::endl;
        }
    });

    // Simulate link flapping
    std::cout << "[INTERRUPT] Simulating " << expected_flaps << " link flaps..." << std::endl;
    if (!simulateLinkFlap(test_port, expected_flaps)) {
        std::cerr << "[INTERRUPT] Failed to simulate link flaps" << std::endl;
        return false;
    }

    // Wait for all events to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Verify we detected the expected number of flap events
    int expected_events = expected_flaps * 2; // Each flap = down + up
    if (flap_count < expected_events) {
        std::cerr << "[INTERRUPT] Expected " << expected_events << " flap events, detected "
                  << flap_count << std::endl;
        return false;
    }

    // Verify final state is UP
    if (!verifySONiCPortStatus(test_port, LinkStatus::UP)) {
        std::cerr << "[INTERRUPT] Final port status is not UP after flapping" << std::endl;
        return false;
    }

    std::cout << "[INTERRUPT] Link flap detection test completed successfully" << std::endl;
    return true;
}

bool SONiCInterruptController::testSONiCCLIResponse() {
    std::cout << "\n[INTERRUPT] Testing SONiC CLI Response to Cable Events..." << std::endl;

    // Get a test port
    auto test_ports = InterruptUtils::getTestPorts(1);
    if (test_ports.empty()) {
        std::cerr << "[INTERRUPT] No test ports available" << std::endl;
        return false;
    }

    std::string test_port = test_ports[0];
    std::cout << "[INTERRUPT] Using test port: " << test_port << std::endl;

    // Test 1: Cable insertion and CLI response
    std::cout << "[INTERRUPT] Test 1: Cable insertion and CLI response..." << std::endl;

    // Get initial CLI output
    std::string initial_status = getSONiCInterfaceStatus(test_port);
    std::cout << "[INTERRUPT] Initial interface status:\n" << initial_status << std::endl;

    // Simulate cable insertion
    if (!simulateCableInsertion(test_port)) {
        std::cerr << "[INTERRUPT] Failed to simulate cable insertion" << std::endl;
        return false;
    }

    // Wait for SONiC to process the change
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Get updated CLI output
    std::string updated_status = getSONiCInterfaceStatus(test_port);
    std::cout << "[INTERRUPT] Updated interface status:\n" << updated_status << std::endl;

    // Verify CLI shows the change
    if (updated_status.find("up") == std::string::npos) {
        std::cerr << "[INTERRUPT] SONiC CLI does not show interface as up" << std::endl;
        return false;
    }

    // Test 2: Cable removal and CLI response
    std::cout << "[INTERRUPT] Test 2: Cable removal and CLI response..." << std::endl;

    // Simulate cable removal
    if (!simulateCableRemoval(test_port)) {
        std::cerr << "[INTERRUPT] Failed to simulate cable removal" << std::endl;
        return false;
    }

    // Wait for SONiC to process the change
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Get final CLI output
    std::string final_status = getSONiCInterfaceStatus(test_port);
    std::cout << "[INTERRUPT] Final interface status:\n" << final_status << std::endl;

    // Verify CLI shows the change
    if (final_status.find("down") == std::string::npos) {
        std::cerr << "[INTERRUPT] SONiC CLI does not show interface as down" << std::endl;
        return false;
    }

    // Test 3: Transceiver information
    std::cout << "[INTERRUPT] Test 3: Transceiver information..." << std::endl;

    std::string transceiver_info = getSONiCTransceiverInfo(test_port);
    std::cout << "[INTERRUPT] Transceiver info:\n" << transceiver_info << std::endl;

    std::cout << "[INTERRUPT] SONiC CLI response test completed successfully" << std::endl;
    return true;
}

bool SONiCInterruptController::testMultiPortEvents() {
    std::cout << "\n[INTERRUPT] Testing Multi-Port Events..." << std::endl;

    // Get multiple test ports
    auto test_ports = InterruptUtils::getTestPorts(4);
    if (test_ports.size() < 2) {
        std::cerr << "[INTERRUPT] Need at least 2 test ports" << std::endl;
        return false;
    }

    std::cout << "[INTERRUPT] Using test ports: ";
    for (const auto& port : test_ports) {
        std::cout << port << " ";
    }
    std::cout << std::endl;

    // Track events for each port
    std::map<std::string, int> port_event_counts;
    for (const auto& port : test_ports) {
        port_event_counts[port] = 0;
    }

    registerGlobalEventHandler([&port_event_counts, this](const PortEvent& event) {
        if (port_event_counts.find(event.port_name) != port_event_counts.end()) {
            port_event_counts[event.port_name]++;
            std::cout << "[INTERRUPT] Event on " << event.port_name << ": "
                      << this->cableEventToString(event.event_type) << std::endl;
        }
    });

    // Simulate simultaneous cable insertions
    std::cout << "[INTERRUPT] Simulating simultaneous cable insertions..." << std::endl;
    std::vector<std::thread> insertion_threads;

    for (const auto& port : test_ports) {
        insertion_threads.emplace_back([this, port]() {
            simulateCableInsertion(port);
        });
    }

    // Wait for all insertions to complete
    for (auto& thread : insertion_threads) {
        thread.join();
    }

    // Wait for event processing
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Verify all ports show up
    for (const auto& port : test_ports) {
        if (!verifySONiCPortStatus(port, LinkStatus::UP)) {
            std::cerr << "[INTERRUPT] Port " << port << " is not up" << std::endl;
            return false;
        }
    }

    // Simulate simultaneous cable removals
    std::cout << "[INTERRUPT] Simulating simultaneous cable removals..." << std::endl;
    std::vector<std::thread> removal_threads;

    for (const auto& port : test_ports) {
        removal_threads.emplace_back([this, port]() {
            simulateCableRemoval(port);
        });
    }

    // Wait for all removals to complete
    for (auto& thread : removal_threads) {
        thread.join();
    }

    // Wait for event processing
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Verify all ports show down
    for (const auto& port : test_ports) {
        if (!verifySONiCPortStatus(port, LinkStatus::DOWN)) {
            std::cerr << "[INTERRUPT] Port " << port << " is not down" << std::endl;
            return false;
        }
    }

    // Verify event counts
    for (const auto& port : test_ports) {
        if (port_event_counts[port] < 2) { // At least insertion + removal
            std::cerr << "[INTERRUPT] Port " << port << " did not generate expected events" << std::endl;
            return false;
        }
    }

    std::cout << "[INTERRUPT] Multi-port events test completed successfully" << std::endl;
    return true;
}

bool SONiCInterruptController::testEventTiming() {
    std::cout << "\n[INTERRUPT] Testing Event Timing..." << std::endl;

    // Get a test port
    auto test_ports = InterruptUtils::getTestPorts(1);
    if (test_ports.empty()) {
        std::cerr << "[INTERRUPT] No test ports available" << std::endl;
        return false;
    }

    std::string test_port = test_ports[0];
    std::cout << "[INTERRUPT] Using test port: " << test_port << std::endl;

    // Track event timing
    std::chrono::system_clock::time_point insertion_time;
    std::chrono::system_clock::time_point event_time;
    bool event_received = false;

    registerEventHandler(CableEvent::CABLE_INSERTED,
        [&event_time, &event_received, test_port](const PortEvent& event) {
            if (event.port_name == test_port) {
                event_time = event.timestamp;
                event_received = true;
                std::cout << "[INTERRUPT] Cable insertion event received" << std::endl;
            }
        });

    // Record insertion time and simulate
    insertion_time = std::chrono::system_clock::now();
    if (!simulateCableInsertion(test_port)) {
        std::cerr << "[INTERRUPT] Failed to simulate cable insertion" << std::endl;
        return false;
    }

    // Wait for event
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!event_received) {
        std::cerr << "[INTERRUPT] Event was not received" << std::endl;
        return false;
    }

    // Calculate timing
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(event_time - insertion_time);
    std::cout << "[INTERRUPT] Event processing time: " << duration.count() << " ms" << std::endl;

    // Verify timing is reasonable (should be < 2 seconds)
    if (duration.count() > 2000) {
        std::cerr << "[INTERRUPT] Event processing took too long: " << duration.count() << " ms" << std::endl;
        return false;
    }

    std::cout << "[INTERRUPT] Event timing test completed successfully" << std::endl;
    return true;
}

// Helper Functions Implementation
bool SONiCInterruptController::validatePortName(const std::string& port_name) {
    std::regex ethernet_pattern("^Ethernet[0-9]+$");
    return std::regex_match(port_name, ethernet_pattern);
}

LinkStatus SONiCInterruptController::parseSONiCLinkStatus(const std::string& status_str) {
    if (status_str == "up") {
        return LinkStatus::UP;
    } else if (status_str == "down") {
        return LinkStatus::DOWN;
    } else {
        return LinkStatus::UNKNOWN;
    }
}

std::string SONiCInterruptController::linkStatusToString(LinkStatus status) {
    switch (status) {
        case LinkStatus::UP: return "UP";
        case LinkStatus::DOWN: return "DOWN";
        case LinkStatus::UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

std::string SONiCInterruptController::cableEventToString(CableEvent event) {
    switch (event) {
        case CableEvent::CABLE_INSERTED: return "CABLE_INSERTED";
        case CableEvent::CABLE_REMOVED: return "CABLE_REMOVED";
        case CableEvent::LINK_UP: return "LINK_UP";
        case CableEvent::LINK_DOWN: return "LINK_DOWN";
        case CableEvent::SFP_INSERTED: return "SFP_INSERTED";
        case CableEvent::SFP_REMOVED: return "SFP_REMOVED";
        case CableEvent::SPEED_CHANGE: return "SPEED_CHANGE";
        case CableEvent::DUPLEX_CHANGE: return "DUPLEX_CHANGE";
        default: return "UNKNOWN_EVENT";
    }
}

void SONiCInterruptController::updateEventStatistics(CableEvent event) {
    std::string event_name = cableEventToString(event);
    m_event_statistics[event_name]++;
}

void SONiCInterruptController::logEvent(const PortEvent& event) {
    auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
    std::cout << "[INTERRUPT] Event logged: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
              << " - " << event.port_name << " - " << cableEventToString(event.event_type)
              << " (" << linkStatusToString(event.old_status) << " -> "
              << linkStatusToString(event.new_status) << ")" << std::endl;
}

// Utility Functions Implementation
std::vector<std::string> InterruptUtils::getTestPorts(int count) {
    std::vector<std::string> ports;
    // Use actual SONiC port names (every 4th port starting from 0)
    for (int i = 0; i < count && i < 32; ++i) {
        ports.push_back("Ethernet" + std::to_string(i * 4));
    }
    return ports;
}

SFPInfo InterruptUtils::generateTestSFPInfo(const std::string& port_name) {
    SFPInfo info;
    info.port_name = port_name;
    info.is_present = true;
    info.vendor_name = "Test Vendor";
    info.part_number = "TEST-SFP-001";
    info.serial_number = "TST" + port_name.substr(8); // Extract number from EthernetX
    info.connector_type = "LC";
    info.cable_length = "1m";
    info.supported_speeds = {1000, 10000, 25000, 100000};
    info.status = "OK";
    return info;
}

bool InterruptUtils::compareTimestamps(const std::chrono::system_clock::time_point& t1,
                                      const std::chrono::system_clock::time_point& t2,
                                      int tolerance_ms) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        t1 > t2 ? t1 - t2 : t2 - t1);
    return duration.count() <= tolerance_ms;
}

} // namespace interrupts
} // namespace sonic
