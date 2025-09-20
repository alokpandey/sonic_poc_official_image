/**
 * @file sai_vlan_manager.h
 * @brief SONiC SAI VLAN Manager Header
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#ifndef SONIC_SAI_VLAN_MANAGER_H
#define SONIC_SAI_VLAN_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Real SAI headers
extern "C" {
#include "sai.h"
#include "saivlan.h"
#include "saibridge.h"
#include "saiport.h"
#include "saiswitch.h"
}

// Forward declaration
namespace sonic {
namespace sai {
class SAIAdapter;
}
}

namespace sonic {
namespace sai {

/**
 * @brief VLAN status enumeration
 */
enum class VLANStatus {
    ACTIVE,
    INACTIVE,
    ERROR
};

/**
 * @brief VLAN member information
 */
struct VLANMember {
    std::string port_name;
    sai_object_id_t port_oid;
    sai_object_id_t member_oid;
    bool tagged;
    std::string added_at;
};

/**
 * @brief VLAN information structure
 */
struct VLANInfo {
    uint16_t vlan_id;
    sai_object_id_t vlan_oid;
    std::string name;
    std::string description;
    VLANStatus status;
    std::vector<VLANMember> members;
    std::string created_at;
    
    VLANInfo() : vlan_id(0), vlan_oid(SAI_NULL_OBJECT_ID), status(VLANStatus::INACTIVE) {}
};

/**
 * @brief SAI VLAN Manager class
 * 
 * This class provides a C++ interface for managing VLANs using the SAI API.
 * It handles VLAN creation, deletion, port membership, and validation.
 */
class SAIVLANManager {
public:
    /**
     * @brief Constructor
     */
    SAIVLANManager();
    
    /**
     * @brief Destructor
     */
    ~SAIVLANManager();
    
    /**
     * @brief Create a new VLAN
     * @param vlan_id VLAN ID (1-4094)
     * @param name Optional VLAN name
     * @return true if successful, false otherwise
     */
    bool createVLAN(uint16_t vlan_id, const std::string& name = "");
    
    /**
     * @brief Delete a VLAN
     * @param vlan_id VLAN ID to delete
     * @return true if successful, false otherwise
     */
    bool deleteVLAN(uint16_t vlan_id);
    
    /**
     * @brief Add a port to VLAN
     * @param vlan_id VLAN ID
     * @param port_name Port name (e.g., "Ethernet0")
     * @param tagged Whether the port should be tagged or untagged
     * @return true if successful, false otherwise
     */
    bool addPortToVLAN(uint16_t vlan_id, const std::string& port_name, bool tagged = false);
    
    /**
     * @brief Remove a port from VLAN
     * @param vlan_id VLAN ID
     * @param port_name Port name to remove
     * @return true if successful, false otherwise
     */
    bool removePortFromVLAN(uint16_t vlan_id, const std::string& port_name);
    
    /**
     * @brief Validate VLAN isolation between two VLANs
     * @param vlan1_id First VLAN ID
     * @param vlan2_id Second VLAN ID
     * @return true if isolation is maintained, false otherwise
     */
    bool validateVLANIsolation(uint16_t vlan1_id, uint16_t vlan2_id);
    
    /**
     * @brief Get all VLANs
     * @return Vector of all VLAN information
     */
    std::vector<VLANInfo> getAllVLANs() const;
    
    /**
     * @brief Get specific VLAN information
     * @param vlan_id VLAN ID to query
     * @return VLAN information structure
     */
    VLANInfo getVLANInfo(uint16_t vlan_id) const;
    
    /**
     * @brief Print VLAN status to console
     */
    void printVLANStatus() const;
    
    /**
     * @brief Check if manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return initialized_; }

private:
    
    /**
     * @brief Get port OID from port name
     * @param port_name Port name
     * @return SAI object ID for the port
     */
    sai_object_id_t getPortOID(const std::string& port_name);
    
    /**
     * @brief Get current timestamp as string
     * @return Formatted timestamp string
     */
    std::string getCurrentTimestamp() const;
    
    /**
     * @brief Cleanup resources
     */
    void cleanup();
    
    // Member variables
    bool initialized_;
    SAIAdapter* sai_adapter_;

    // VLAN storage
    std::map<uint16_t, VLANInfo> vlans_;
    
    // Disable copy constructor and assignment operator
    SAIVLANManager(const SAIVLANManager&) = delete;
    SAIVLANManager& operator=(const SAIVLANManager&) = delete;
};

} // namespace sai
} // namespace sonic

#endif // SONIC_SAI_VLAN_MANAGER_H
