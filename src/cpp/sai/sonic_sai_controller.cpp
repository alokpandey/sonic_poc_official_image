#include "sonic_sai_controller.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <regex>
#include <iomanip>

namespace sonic {
namespace sai {

SONiCSAIController::SONiCSAIController() 
    : m_initialized(false), m_sonic_container_name("sonic-vs-official"), m_next_object_id(1000) {
}

SONiCSAIController::~SONiCSAIController() {
    cleanup();
}

bool SONiCSAIController::initialize() {
    std::cout << "[SAI] Initializing SONiC SAI Controller..." << std::endl;
    
    // Test connection to SONiC container
    std::string output;
    if (!executeSONiCCommand("echo 'SAI_TEST'", output)) {
        std::cerr << "[SAI] Failed to connect to SONiC container" << std::endl;
        return false;
    }
    
    // Initialize caches
    if (!refreshPortCache()) {
        std::cerr << "[SAI] Failed to initialize port cache" << std::endl;
        return false;
    }
    
    if (!refreshVLANCache()) {
        std::cerr << "[SAI] Failed to initialize VLAN cache" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "[SAI] SONiC SAI Controller initialized successfully" << std::endl;
    std::cout << "[SAI] Found " << m_port_cache.size() << " ports" << std::endl;
    std::cout << "[SAI] Found " << m_vlan_cache.size() << " VLANs" << std::endl;
    
    return true;
}

void SONiCSAIController::cleanup() {
    if (m_initialized) {
        std::cout << "[SAI] Cleaning up SONiC SAI Controller..." << std::endl;
        m_initialized = false;
    }
}

bool SONiCSAIController::executeSONiCCommand(const std::string& command, std::string& output) {
    // Use a simpler approach without bash -c to avoid escaping issues
    std::string full_command = "docker exec " + m_sonic_container_name + " " + command;

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        std::cerr << "[SAI] Failed to execute: " << full_command << std::endl;
        return false;
    }

    char buffer[256];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int result = pclose(pipe);
    if (result != 0) {
        std::cerr << "[SAI] Command failed: " << full_command << " (exit code: " << result << ")" << std::endl;
    }
    return (result == 0);
}

bool SONiCSAIController::executeRedisCommand(const std::string& command, int db_id, std::string& output) {
    std::stringstream ss;
    ss << "redis-cli -n " << db_id << " " << command;
    std::string redis_command = ss.str();

    // Debug output
    std::cout << "[SAI] Executing Redis command: " << redis_command << std::endl;

    return executeSONiCCommand(redis_command, output);
}

bool SONiCSAIController::setRedisValue(const std::string& key, const std::string& value, int db_id) {
    std::string output;
    std::stringstream ss;
    ss << "SET '" << key << "' '" << value << "'";
    return executeRedisCommand(ss.str(), db_id, output);
}

std::string SONiCSAIController::getRedisValue(const std::string& key, int db_id) {
    std::string output;
    std::stringstream ss;
    ss << "GET '" << key << "'";
    if (executeRedisCommand(ss.str(), db_id, output)) {
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    return "";
}

bool SONiCSAIController::setRedisHashField(const std::string& key, const std::string& field, const std::string& value, int db_id) {
    std::string output;
    std::stringstream ss;
    ss << "HSET \"" << key << "\" \"" << field << "\" \"" << value << "\"";
    return executeRedisCommand(ss.str(), db_id, output);
}

std::string SONiCSAIController::getRedisHashField(const std::string& key, const std::string& field, int db_id) {
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

// VLAN Management Implementation
bool SONiCSAIController::createVLAN(uint16_t vlan_id, const std::string& name) {
    std::cout << "[SAI] Creating VLAN " << vlan_id;
    if (!name.empty()) {
        std::cout << " with name '" << name << "'";
    }
    std::cout << std::endl;
    
    if (!validateVLANID(vlan_id)) {
        std::cerr << "[SAI] Invalid VLAN ID: " << vlan_id << std::endl;
        return false;
    }
    
    // Check if VLAN already exists and delete it first (for test cleanup)
    if (m_vlan_cache.find(vlan_id) != m_vlan_cache.end()) {
        std::cout << "[SAI] VLAN " << vlan_id << " already exists, deleting first..." << std::endl;
        deleteVLAN(vlan_id, true); // Silent deletion
    }
    
    // Create VLAN using SONiC config command
    std::string command = "config vlan add " + std::to_string(vlan_id);
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Update CONFIG_DB
        std::string vlan_key = "VLAN|Vlan" + std::to_string(vlan_id);
        setRedisHashField(vlan_key, "vlanid", std::to_string(vlan_id), 4);
        
        if (!name.empty()) {
            setRedisHashField(vlan_key, "description", name, 4);
        }
        
        // Update cache
        VLANInfo vlan_info;
        vlan_info.vlan_id = vlan_id;
        vlan_info.name = name.empty() ? ("Vlan" + std::to_string(vlan_id)) : name;
        vlan_info.is_active = true;
        vlan_info.description = name;
        m_vlan_cache[vlan_id] = vlan_info;
        
        std::cout << "[SAI] VLAN " << vlan_id << " created successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to create VLAN " << vlan_id << ": " << output << std::endl;
    }
    
    return result;
}

bool SONiCSAIController::deleteVLAN(uint16_t vlan_id) {
    return deleteVLAN(vlan_id, false);
}

bool SONiCSAIController::deleteVLAN(uint16_t vlan_id, bool silent) {
    if (!silent) {
        std::cout << "[SAI] Deleting VLAN " << vlan_id << std::endl;
    }

    if (!validateVLANID(vlan_id)) {
        if (!silent) {
            std::cerr << "[SAI] Invalid VLAN ID: " << vlan_id << std::endl;
        }
        return false;
    }

    // Check if VLAN exists
    if (m_vlan_cache.find(vlan_id) == m_vlan_cache.end()) {
        if (!silent) {
            std::cerr << "[SAI] VLAN " << vlan_id << " does not exist" << std::endl;
        }
        return false;
    }

    // Remove all ports from VLAN first
    VLANInfo& vlan_info = m_vlan_cache[vlan_id];
    for (const auto& port : vlan_info.member_ports) {
        removePortFromVLAN(vlan_id, port);
    }

    // Delete VLAN using SONiC config command
    std::string command = "config vlan del " + std::to_string(vlan_id);
    std::string output;
    bool result = executeSONiCCommand(command, output);

    if (result) {
        // Remove from CONFIG_DB
        std::string vlan_key = "VLAN|Vlan" + std::to_string(vlan_id);
        std::string del_command = "DEL '" + vlan_key + "'";
        executeRedisCommand(del_command, 4, output);

        // Update cache
        m_vlan_cache.erase(vlan_id);

        if (!silent) {
            std::cout << "[SAI] VLAN " << vlan_id << " deleted successfully" << std::endl;
        }
    } else {
        if (!silent) {
            std::cerr << "[SAI] Failed to delete VLAN " << vlan_id << ": " << output << std::endl;
        }
    }

    return result;
}

bool SONiCSAIController::addPortToVLAN(uint16_t vlan_id, const std::string& port_name, bool tagged) {
    std::cout << "[SAI] Adding port " << port_name << " to VLAN " << vlan_id 
              << " (" << (tagged ? "tagged" : "untagged") << ")" << std::endl;
    
    if (!validateVLANID(vlan_id) || !validatePortName(port_name)) {
        std::cerr << "[SAI] Invalid VLAN ID or port name" << std::endl;
        return false;
    }
    
    // Check if VLAN exists
    if (m_vlan_cache.find(vlan_id) == m_vlan_cache.end()) {
        std::cerr << "[SAI] VLAN " << vlan_id << " does not exist" << std::endl;
        return false;
    }
    
    // Add port to VLAN using SONiC config command
    std::string command = "config vlan member add ";
    if (!tagged) {
        command += "-u ";  // -u for untagged, no flag for tagged (default)
    }
    command += std::to_string(vlan_id) + " " + port_name;
    
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Update CONFIG_DB
        std::string member_key = "VLAN_MEMBER|Vlan" + std::to_string(vlan_id) + "|" + port_name;
        setRedisHashField(member_key, "tagging_mode", tagged ? "tagged" : "untagged", 4);
        
        // Update cache
        VLANInfo& vlan_info = m_vlan_cache[vlan_id];
        vlan_info.member_ports.push_back(port_name);
        if (tagged) {
            vlan_info.tagged_ports.push_back(port_name);
        } else {
            vlan_info.untagged_ports.push_back(port_name);
        }
        
        // Update port cache
        if (m_port_cache.find(port_name) != m_port_cache.end()) {
            m_port_cache[port_name].vlan_memberships.push_back(vlan_id);
        }
        
        std::cout << "[SAI] Port " << port_name << " added to VLAN " << vlan_id << " successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to add port " << port_name << " to VLAN " << vlan_id << ": " << output << std::endl;
    }
    
    return result;
}

bool SONiCSAIController::removePortFromVLAN(uint16_t vlan_id, const std::string& port_name) {
    std::cout << "[SAI] Removing port " << port_name << " from VLAN " << vlan_id << std::endl;
    
    if (!validateVLANID(vlan_id) || !validatePortName(port_name)) {
        std::cerr << "[SAI] Invalid VLAN ID or port name" << std::endl;
        return false;
    }
    
    // Remove port from VLAN using SONiC config command
    std::string command = "config vlan member del " + std::to_string(vlan_id) + " " + port_name;
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Remove from CONFIG_DB
        std::string member_key = "VLAN_MEMBER|Vlan" + std::to_string(vlan_id) + "|" + port_name;
        std::string del_command = "DEL '" + member_key + "'";
        executeRedisCommand(del_command, 4, output);
        
        // Update cache
        if (m_vlan_cache.find(vlan_id) != m_vlan_cache.end()) {
            VLANInfo& vlan_info = m_vlan_cache[vlan_id];
            vlan_info.member_ports.erase(
                std::remove(vlan_info.member_ports.begin(), vlan_info.member_ports.end(), port_name),
                vlan_info.member_ports.end());
            vlan_info.tagged_ports.erase(
                std::remove(vlan_info.tagged_ports.begin(), vlan_info.tagged_ports.end(), port_name),
                vlan_info.tagged_ports.end());
            vlan_info.untagged_ports.erase(
                std::remove(vlan_info.untagged_ports.begin(), vlan_info.untagged_ports.end(), port_name),
                vlan_info.untagged_ports.end());
        }
        
        // Update port cache
        if (m_port_cache.find(port_name) != m_port_cache.end()) {
            auto& vlans = m_port_cache[port_name].vlan_memberships;
            vlans.erase(std::remove(vlans.begin(), vlans.end(), vlan_id), vlans.end());
        }
        
        std::cout << "[SAI] Port " << port_name << " removed from VLAN " << vlan_id << " successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to remove port " << port_name << " from VLAN " << vlan_id << ": " << output << std::endl;
    }
    
    return result;
}

VLANInfo SONiCSAIController::getVLANInfo(uint16_t vlan_id) {
    auto it = m_vlan_cache.find(vlan_id);
    if (it != m_vlan_cache.end()) {
        return it->second;
    }
    
    // Return empty VLAN info if not found
    VLANInfo empty_vlan;
    empty_vlan.vlan_id = 0;
    return empty_vlan;
}

std::vector<VLANInfo> SONiCSAIController::getAllVLANs() {
    std::vector<VLANInfo> vlans;
    for (const auto& pair : m_vlan_cache) {
        vlans.push_back(pair.second);
    }
    return vlans;
}

bool SONiCSAIController::setVLANDescription(uint16_t vlan_id, const std::string& description) {
    std::cout << "[SAI] Setting VLAN " << vlan_id << " description to: " << description << std::endl;
    
    if (m_vlan_cache.find(vlan_id) == m_vlan_cache.end()) {
        std::cerr << "[SAI] VLAN " << vlan_id << " does not exist" << std::endl;
        return false;
    }
    
    // Update CONFIG_DB
    std::string vlan_key = "VLAN|Vlan" + std::to_string(vlan_id);
    bool result = setRedisHashField(vlan_key, "description", description, 4);
    
    if (result) {
        // Update cache
        m_vlan_cache[vlan_id].description = description;
        std::cout << "[SAI] VLAN " << vlan_id << " description updated successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to update VLAN " << vlan_id << " description" << std::endl;
    }
    
    return result;
}

// Port Management Implementation
bool SONiCSAIController::setPortAdminStatus(const std::string& port_name, bool up) {
    std::cout << "[SAI] Setting port " << port_name << " admin status to " 
              << (up ? "UP" : "DOWN") << std::endl;
    
    if (!validatePortName(port_name)) {
        std::cerr << "[SAI] Invalid port name: " << port_name << std::endl;
        return false;
    }
    
    std::string command;
    if (up) {
        command = "config interface startup " + port_name;
    } else {
        command = "config interface shutdown " + port_name;
    }
    
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Update CONFIG_DB
        std::string port_key = "PORT|" + port_name;
        setRedisHashField(port_key, "admin_status", up ? "up" : "down", 4);
        
        // Update cache
        if (m_port_cache.find(port_name) != m_port_cache.end()) {
            m_port_cache[port_name].admin_status = up ? "up" : "down";
        }
        
        std::cout << "[SAI] Port " << port_name << " admin status updated successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to update port " << port_name << " admin status: " << output << std::endl;
    }
    
    return result;
}

bool SONiCSAIController::setPortSpeed(const std::string& port_name, uint32_t speed) {
    std::cout << "[SAI] Setting port " << port_name << " speed to " << speed << " Mbps" << std::endl;
    
    if (!validatePortName(port_name)) {
        std::cerr << "[SAI] Invalid port name: " << port_name << std::endl;
        return false;
    }
    
    std::string command = "config interface speed " + port_name + " " + std::to_string(speed);
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Update CONFIG_DB
        std::string port_key = "PORT|" + port_name;
        setRedisHashField(port_key, "speed", std::to_string(speed), 4);
        
        // Update cache
        if (m_port_cache.find(port_name) != m_port_cache.end()) {
            m_port_cache[port_name].speed = speed;
        }
        
        std::cout << "[SAI] Port " << port_name << " speed updated successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to update port " << port_name << " speed: " << output << std::endl;
    }
    
    return result;
}

bool SONiCSAIController::setPortMTU(const std::string& port_name, uint32_t mtu) {
    std::cout << "[SAI] Setting port " << port_name << " MTU to " << mtu << " bytes" << std::endl;
    
    if (!validatePortName(port_name)) {
        std::cerr << "[SAI] Invalid port name: " << port_name << std::endl;
        return false;
    }
    
    std::string command = "config interface mtu " + port_name + " " + std::to_string(mtu);
    std::string output;
    bool result = executeSONiCCommand(command, output);
    
    if (result) {
        // Update CONFIG_DB
        std::string port_key = "PORT|" + port_name;
        setRedisHashField(port_key, "mtu", std::to_string(mtu), 4);
        
        // Update cache
        if (m_port_cache.find(port_name) != m_port_cache.end()) {
            m_port_cache[port_name].mtu = mtu;
        }
        
        std::cout << "[SAI] Port " << port_name << " MTU updated successfully" << std::endl;
    } else {
        std::cerr << "[SAI] Failed to update port " << port_name << " MTU: " << output << std::endl;
    }
    
    return result;
}

// Validation Helper Functions
bool SONiCSAIController::validateVLANID(uint16_t vlan_id) {
    return (vlan_id >= 1 && vlan_id <= 4094);
}

bool SONiCSAIController::validatePortName(const std::string& port_name) {
    // Check if port name matches Ethernet pattern
    std::regex ethernet_pattern("^Ethernet[0-9]+$");
    return std::regex_match(port_name, ethernet_pattern);
}

bool SONiCSAIController::validateMACAddress(const std::string& mac_address) {
    std::regex mac_pattern("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
    return std::regex_match(mac_address, mac_pattern);
}

bool SONiCSAIController::validateIPAddress(const std::string& ip_address) {
    std::regex ipv4_pattern("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(ip_address, ipv4_pattern);
}

// Cache Refresh Functions
bool SONiCSAIController::refreshPortCache() {
    std::cout << "[SAI] Refreshing port cache..." << std::endl;

    m_port_cache.clear();

    // Get all ports from CONFIG_DB
    std::string output;
    if (!executeRedisCommand("KEYS \"PORT|*\"", 4, output)) {
        std::cout << "[SAI] Failed to get port keys from Redis" << std::endl;
        return false;
    }

    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("PORT|") == 0) {
            std::string port_name = line.substr(5); // Remove "PORT|" prefix

            PortInfo port_info;
            port_info.port_name = port_name;
            port_info.port_id = static_cast<uint32_t>(m_port_cache.size() + 1);

            // Get port details from Redis
            std::string port_key = "PORT|" + port_name;
            std::string speed_str = getRedisHashField(port_key, "speed", 4);
            std::string mtu_str = getRedisHashField(port_key, "mtu", 4);

            port_info.speed = speed_str.empty() ? 100000 : std::stoul(speed_str);
            port_info.mtu = mtu_str.empty() ? 9100 : std::stoul(mtu_str);
            port_info.admin_status = getRedisHashField(port_key, "admin_status", 4);

            // Get operational status from APPL_DB
            std::string appl_key = "PORT_TABLE:" + port_name;
            port_info.oper_status = getRedisHashField(appl_key, "oper_status", 0);

            m_port_cache[port_name] = port_info;
        }
    }

    std::cout << "[SAI] Port cache refreshed: " << m_port_cache.size() << " ports" << std::endl;
    return true;
}

bool SONiCSAIController::refreshVLANCache() {
    std::cout << "[SAI] Refreshing VLAN cache..." << std::endl;

    m_vlan_cache.clear();

    // Get all VLANs from CONFIG_DB
    std::string output;
    if (!executeRedisCommand("KEYS \"VLAN|*\"", 4, output)) {
        std::cout << "[SAI] Failed to get VLAN keys from Redis" << std::endl;
        return false;
    }

    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("VLAN|Vlan") == 0) {
            std::string vlan_str = line.substr(9); // Remove "VLAN|Vlan" prefix
            uint16_t vlan_id = static_cast<uint16_t>(std::stoul(vlan_str));

            VLANInfo vlan_info;
            vlan_info.vlan_id = vlan_id;
            vlan_info.name = "Vlan" + std::to_string(vlan_id);
            vlan_info.is_active = true;

            // Get VLAN description
            std::string vlan_key = "VLAN|Vlan" + std::to_string(vlan_id);
            vlan_info.description = getRedisHashField(vlan_key, "description", 4);

            // Get VLAN members
            std::string member_output;
            std::string member_pattern = "VLAN_MEMBER|Vlan" + std::to_string(vlan_id) + "|*";
            if (executeRedisCommand("KEYS '" + member_pattern + "'", 4, member_output)) {
                std::istringstream member_iss(member_output);
                std::string member_line;
                while (std::getline(member_iss, member_line)) {
                    size_t last_pipe = member_line.find_last_of('|');
                    if (last_pipe != std::string::npos) {
                        std::string port_name = member_line.substr(last_pipe + 1);
                        vlan_info.member_ports.push_back(port_name);

                        // Check if tagged or untagged
                        std::string tagging_mode = getRedisHashField(member_line, "tagging_mode", 4);
                        if (tagging_mode == "tagged") {
                            vlan_info.tagged_ports.push_back(port_name);
                        } else {
                            vlan_info.untagged_ports.push_back(port_name);
                        }
                    }
                }
            }

            m_vlan_cache[vlan_id] = vlan_info;
        }
    }

    std::cout << "[SAI] VLAN cache refreshed: " << m_vlan_cache.size() << " VLANs" << std::endl;
    return true;
}

uint32_t SONiCSAIController::generateObjectID(SAIObjectType type) {
    uint32_t object_id = m_next_object_id++;
    m_object_type_map[object_id] = type;
    return object_id;
}

bool SONiCSAIController::isValidObjectID(uint32_t object_id, SAIObjectType expected_type) {
    auto it = m_object_type_map.find(object_id);
    return (it != m_object_type_map.end() && it->second == expected_type);
}

PortInfo SONiCSAIController::getPortInfo(const std::string& port_name) {
    auto it = m_port_cache.find(port_name);
    if (it != m_port_cache.end()) {
        return it->second;
    }

    // Return empty port info if not found
    PortInfo empty_port;
    empty_port.port_name = "";
    empty_port.port_id = 0;
    return empty_port;
}

std::vector<PortInfo> SONiCSAIController::getAllPorts() {
    std::vector<PortInfo> ports;
    for (const auto& pair : m_port_cache) {
        ports.push_back(pair.second);
    }
    return ports;
}

} // namespace sai
} // namespace sonic
