#ifndef SONIC_INTERRUPT_CONTROLLER_H
#define SONIC_INTERRUPT_CONTROLLER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

namespace sonic {
namespace interrupts {

// Link Status Types
enum class LinkStatus {
    UP,
    DOWN,
    UNKNOWN
};

// Cable Event Types
enum class CableEvent {
    CABLE_INSERTED,
    CABLE_REMOVED,
    LINK_UP,
    LINK_DOWN,
    SFP_INSERTED,
    SFP_REMOVED,
    SPEED_CHANGE,
    DUPLEX_CHANGE
};

// Port Event Information
struct PortEvent {
    std::string port_name;
    CableEvent event_type;
    LinkStatus old_status;
    LinkStatus new_status;
    uint32_t speed_mbps;
    std::string duplex;
    std::chrono::system_clock::time_point timestamp;
    std::string additional_info;
};

// SFP/Transceiver Information
struct SFPInfo {
    std::string port_name;
    bool is_present;
    std::string vendor_name;
    std::string part_number;
    std::string serial_number;
    std::string connector_type;
    std::string cable_length;
    std::vector<uint32_t> supported_speeds;
    std::string status;
};

// Link State Information
struct LinkState {
    std::string port_name;
    LinkStatus admin_status;
    LinkStatus oper_status;
    uint32_t speed_mbps;
    std::string duplex;
    bool auto_neg;
    uint32_t mtu;
    std::string mac_address;
    std::chrono::system_clock::time_point last_change;
    uint64_t link_up_count;
    uint64_t link_down_count;
};

// Interrupt Handler Callback Type
using InterruptHandler = std::function<void(const PortEvent&)>;

// Main Interrupt Controller Class
class SONiCInterruptController {
public:
    SONiCInterruptController();
    ~SONiCInterruptController();

    // Initialize interrupt monitoring
    bool initialize();
    void cleanup();

    // Cable Event Simulation (for testing)
    bool simulateCableInsertion(const std::string& port_name);
    bool simulateCableRemoval(const std::string& port_name);
    bool simulateLinkFlap(const std::string& port_name, int flap_count = 1);
    bool simulateSFPInsertion(const std::string& port_name, const SFPInfo& sfp_info);
    bool simulateSFPRemoval(const std::string& port_name);

    // Real Hardware Event Detection
    bool startEventMonitoring();
    bool stopEventMonitoring();
    bool isMonitoring() const;

    // Event Handler Registration
    void registerEventHandler(CableEvent event_type, InterruptHandler handler);
    void unregisterEventHandler(CableEvent event_type);
    void registerGlobalEventHandler(InterruptHandler handler);

    // Port Status Queries
    LinkState getPortLinkState(const std::string& port_name);
    std::vector<LinkState> getAllPortStates();
    SFPInfo getSFPInfo(const std::string& port_name);
    std::vector<SFPInfo> getAllSFPInfo();

    // Event History and Statistics
    std::vector<PortEvent> getEventHistory(const std::string& port_name = "");
    std::map<std::string, uint64_t> getEventStatistics();
    void clearEventHistory();

    // Link State Validation
    bool validateLinkState(const std::string& port_name, LinkStatus expected_status, 
                          int timeout_ms = 5000);
    bool waitForLinkEvent(const std::string& port_name, CableEvent expected_event, 
                         int timeout_ms = 10000);

    // SONiC CLI Integration
    bool refreshPortStatusFromSONiC();
    bool verifySONiCPortStatus(const std::string& port_name, LinkStatus expected_status);
    std::string getSONiCInterfaceStatus(const std::string& port_name);
    std::string getSONiCTransceiverInfo(const std::string& port_name);

    // Test Functions
    bool runInterruptTests();
    bool testCableInsertionRemoval();
    bool testLinkFlapDetection();
    bool testSFPHotSwap();
    bool testMultiPortEvents();
    bool testEventTiming();
    bool testSONiCCLIResponse();

private:
    bool m_initialized;
    std::atomic<bool> m_monitoring;
    std::string m_sonic_container_name;
    bool m_verbose_debug;
    
    // Event monitoring thread
    std::unique_ptr<std::thread> m_monitor_thread;
    void monitoringLoop();
    
    // Event handlers
    std::map<CableEvent, std::vector<InterruptHandler>> m_event_handlers;
    std::vector<InterruptHandler> m_global_handlers;
    
    // State tracking
    std::map<std::string, LinkState> m_port_states;
    std::map<std::string, SFPInfo> m_sfp_info;
    std::vector<PortEvent> m_event_history;
    std::map<std::string, uint64_t> m_event_statistics;
    
    // Synchronization
    mutable std::mutex m_state_mutex;
    mutable std::mutex m_event_mutex;
    mutable std::mutex m_handler_mutex;
    
    // Helper functions for SONiC communication
    bool executeSONiCCommand(const std::string& command, std::string& output);
    bool executeRedisCommand(const std::string& command, int db_id, std::string& output);
    bool setRedisValue(const std::string& key, const std::string& value, int db_id = 4);
    std::string getRedisValue(const std::string& key, int db_id = 4);
    bool setRedisHashField(const std::string& key, const std::string& field, 
                          const std::string& value, int db_id = 4);
    std::string getRedisHashField(const std::string& key, const std::string& field, int db_id = 4);
    
    // Port state management
    bool updatePortState(const std::string& port_name);
    bool detectPortChanges();
    void triggerEvent(const PortEvent& event);
    
    // Event simulation helpers
    bool simulatePortStateChange(const std::string& port_name, LinkStatus new_status);
    bool updateSONiCPortStatus(const std::string& port_name, LinkStatus status);
    bool updateSONiCSFPStatus(const std::string& port_name, bool present, const SFPInfo& info = {});
    
    // Validation helpers
    bool validatePortName(const std::string& port_name);
    LinkStatus parseSONiCLinkStatus(const std::string& status_str);
    std::string linkStatusToString(LinkStatus status);
    CableEvent stringToCableEvent(const std::string& event_str);
    std::string cableEventToString(CableEvent event);
    LinkState getPortLinkStateUnsafe(const std::string& port_name); // Assumes mutex is already locked
    
    // Timing and synchronization
    std::chrono::system_clock::time_point m_last_poll_time;
    static constexpr int POLL_INTERVAL_MS = 1000;
    static constexpr int EVENT_TIMEOUT_MS = 5000;
    
    // Statistics tracking
    void updateEventStatistics(CableEvent event);
    void logEvent(const PortEvent& event);
};

// Interrupt Test Scenarios
class InterruptTestScenarios {
public:
    // Real-world cable scenarios
    static bool testDataCenterCableManagement(SONiCInterruptController* controller);
    static bool testNetworkMaintenanceScenario(SONiCInterruptController* controller);
    static bool testHotSwapScenario(SONiCInterruptController* controller);
    
    // Failure scenarios
    static bool testIntermittentConnection(SONiCInterruptController* controller);
    static bool testRapidCableFlapping(SONiCInterruptController* controller);
    static bool testSimultaneousMultiPortEvents(SONiCInterruptController* controller);
    
    // Performance scenarios
    static bool testHighFrequencyEvents(SONiCInterruptController* controller);
    static bool testEventStormHandling(SONiCInterruptController* controller);
    static bool testLongTermStabilityTest(SONiCInterruptController* controller);
};

// Utility Functions
class InterruptUtils {
public:
    static std::vector<std::string> getTestPorts(int count = 4);
    static SFPInfo generateTestSFPInfo(const std::string& port_name);
    static bool compareTimestamps(const std::chrono::system_clock::time_point& t1,
                                 const std::chrono::system_clock::time_point& t2,
                                 int tolerance_ms = 1000);
    static std::string formatEventHistory(const std::vector<PortEvent>& events);
    static std::string formatLinkState(const LinkState& state);
};

} // namespace interrupts
} // namespace sonic

#endif // SONIC_INTERRUPT_CONTROLLER_H
