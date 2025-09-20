#ifndef SONIC_SAI_CONTROLLER_H
#define SONIC_SAI_CONTROLLER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace sonic {
namespace sai {

// SAI Object Types
enum class SAIObjectType {
    SWITCH,
    PORT,
    VLAN,
    VLAN_MEMBER,
    BRIDGE,
    BRIDGE_PORT,
    FDB_ENTRY,
    ROUTE_ENTRY,
    NEXT_HOP,
    NEXT_HOP_GROUP,
    ACL_TABLE,
    ACL_ENTRY
};

// VLAN Information Structure
struct VLANInfo {
    uint16_t vlan_id;
    std::string name;
    std::vector<std::string> member_ports;
    std::vector<std::string> tagged_ports;
    std::vector<std::string> untagged_ports;
    bool is_active;
    std::string description;
};

// Port Information Structure
struct PortInfo {
    std::string port_name;
    uint32_t port_id;
    uint32_t speed;
    uint32_t mtu;
    std::string admin_status;
    std::string oper_status;
    std::vector<uint16_t> vlan_memberships;
    std::string mac_address;
};

// FDB Entry Structure
struct FDBEntry {
    std::string mac_address;
    uint16_t vlan_id;
    std::string port_name;
    std::string entry_type; // static, dynamic
    uint32_t age_time;
};

// Route Entry Structure
struct RouteEntry {
    std::string destination;
    std::string prefix_length;
    std::string next_hop;
    std::string interface;
    uint32_t metric;
    std::string route_type;
};

// ACL Rule Structure
struct ACLRule {
    uint32_t rule_id;
    std::string table_name;
    std::string src_ip;
    std::string dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    std::string protocol;
    std::string action; // permit, deny, redirect
    uint32_t priority;
};

// Main SAI Controller Class
class SONiCSAIController {
public:
    SONiCSAIController();
    ~SONiCSAIController();

    // Initialize SAI connection to SONiC
    bool initialize();
    void cleanup();

    // VLAN Management
    bool createVLAN(uint16_t vlan_id, const std::string& name = "");
    bool deleteVLAN(uint16_t vlan_id);
    bool deleteVLAN(uint16_t vlan_id, bool silent);
    bool addPortToVLAN(uint16_t vlan_id, const std::string& port_name, bool tagged = true);
    bool removePortFromVLAN(uint16_t vlan_id, const std::string& port_name);
    VLANInfo getVLANInfo(uint16_t vlan_id);
    std::vector<VLANInfo> getAllVLANs();
    bool setVLANDescription(uint16_t vlan_id, const std::string& description);

    // Port Management
    bool setPortAdminStatus(const std::string& port_name, bool up);
    bool setPortSpeed(const std::string& port_name, uint32_t speed);
    bool setPortMTU(const std::string& port_name, uint32_t mtu);
    PortInfo getPortInfo(const std::string& port_name);
    std::vector<PortInfo> getAllPorts();

    // Bridge Management
    bool createBridge(const std::string& bridge_name);
    bool deleteBridge(const std::string& bridge_name);
    bool addPortToBridge(const std::string& bridge_name, const std::string& port_name);
    bool removePortFromBridge(const std::string& bridge_name, const std::string& port_name);

    // FDB (Forwarding Database) Management
    bool addStaticFDBEntry(const std::string& mac_address, uint16_t vlan_id, const std::string& port_name);
    bool deleteStaticFDBEntry(const std::string& mac_address, uint16_t vlan_id);
    std::vector<FDBEntry> getFDBEntries(uint16_t vlan_id = 0);
    bool flushFDBEntries(uint16_t vlan_id = 0);

    // Routing Management
    bool addRoute(const std::string& destination, const std::string& prefix_length, 
                  const std::string& next_hop, const std::string& interface = "");
    bool deleteRoute(const std::string& destination, const std::string& prefix_length);
    std::vector<RouteEntry> getRouteTable();

    // ACL Management
    bool createACLTable(const std::string& table_name, const std::string& stage = "ingress");
    bool deleteACLTable(const std::string& table_name);
    bool addACLRule(const ACLRule& rule);
    bool deleteACLRule(uint32_t rule_id, const std::string& table_name);
    std::vector<ACLRule> getACLRules(const std::string& table_name = "");

    // Statistics and Monitoring
    std::map<std::string, uint64_t> getPortStatistics(const std::string& port_name);
    std::map<std::string, uint64_t> getVLANStatistics(uint16_t vlan_id);
    bool clearPortStatistics(const std::string& port_name);

    // Test Functions
    bool runSAIFunctionalTests();
    bool testVLANOperations();
    bool testPortOperations();
    bool testFDBOperations();
    bool testRoutingOperations();
    bool testACLOperations();

    // Advanced SAI Operations
    bool createLAG(const std::string& lag_name, const std::vector<std::string>& member_ports);
    bool deleteLAG(const std::string& lag_name);
    bool addPortToLAG(const std::string& lag_name, const std::string& port_name);
    bool removePortFromLAG(const std::string& lag_name, const std::string& port_name);

    // Redis communication (public for test framework access)
    bool executeRedisCommand(const std::string& command, int db_id, std::string& output);

private:
    bool m_initialized;
    std::string m_sonic_container_name;
    
    // Helper functions for SONiC communication
    bool executeSONiCCommand(const std::string& command, std::string& output);
    bool setRedisValue(const std::string& key, const std::string& value, int db_id = 4);
    std::string getRedisValue(const std::string& key, int db_id = 4);
    bool setRedisHashField(const std::string& key, const std::string& field, const std::string& value, int db_id = 4);
    std::string getRedisHashField(const std::string& key, const std::string& field, int db_id = 4);
    
    // SAI-specific helpers
    bool validateVLANID(uint16_t vlan_id);
    bool validatePortName(const std::string& port_name);
    bool validateMACAddress(const std::string& mac_address);
    bool validateIPAddress(const std::string& ip_address);
    
    // Internal state management
    std::map<uint16_t, VLANInfo> m_vlan_cache;
    std::map<std::string, PortInfo> m_port_cache;
    std::vector<FDBEntry> m_fdb_cache;
    std::vector<RouteEntry> m_route_cache;
    std::vector<ACLRule> m_acl_cache;
    
    // SAI object management
    uint32_t m_next_object_id;
    std::map<uint32_t, SAIObjectType> m_object_type_map;
    
    // Internal helper methods
    bool refreshVLANCache();
    bool refreshPortCache();
    bool refreshFDBCache();
    bool refreshRouteCache();
    bool refreshACLCache();
    
    uint32_t generateObjectID(SAIObjectType type);
    bool isValidObjectID(uint32_t object_id, SAIObjectType expected_type);
};

} // namespace sai
} // namespace sonic

#endif // SONIC_SAI_CONTROLLER_H
