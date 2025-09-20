/**
 * @file sai_vlan_manager.cpp
 * @brief SONiC SAI VLAN Manager Implementation in C++
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#include "sai_vlan_manager.h"
#include "sai_adapter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace sonic {
namespace sai {

SAIVLANManager::SAIVLANManager() : initialized_(false) {
    // Get SAI adapter instance
    sai_adapter_ = SAIAdapter::getInstance();
    if (sai_adapter_ && sai_adapter_->initialize()) {
        initialized_ = true;
        std::cout << "SAI VLAN Manager initialized successfully" << std::endl;
    } else {
        std::cerr << "Failed to initialize SAI VLAN Manager" << std::endl;
    }
}

SAIVLANManager::~SAIVLANManager() {
    cleanup();
}



bool SAIVLANManager::createVLAN(uint16_t vlan_id, const std::string& name) {
    std::cout << "SAIVLANManager::createVLAN called with VLAN ID: " << vlan_id << ", name: " << name << std::endl;
    std::cout << "Initialized status: " << (initialized_ ? "true" : "false") << std::endl;

    if (!initialized_) {
        std::cerr << "SAI not initialized" << std::endl;
        return false;
    }
    
    // Check if VLAN already exists
    if (vlans_.find(vlan_id) != vlans_.end()) {
        std::cerr << "VLAN " << vlan_id << " already exists" << std::endl;
        return false;
    }
    
    sai_attribute_t vlan_attr;
    vlan_attr.id = SAI_VLAN_ATTR_VLAN_ID;
    vlan_attr.value.u16 = vlan_id;
    
    sai_object_id_t vlan_oid;
    sai_status_t status = sai_adapter_->getVLANAPI()->create_vlan(&vlan_oid, sai_adapter_->getSwitchId(), 1, &vlan_attr);
    
    if (status != SAI_STATUS_SUCCESS) {
        std::cerr << "Failed to create VLAN " << vlan_id << ": " << status << std::endl;
        return false;
    }
    
    // Store VLAN information
    VLANInfo vlan_info;
    vlan_info.vlan_id = vlan_id;
    vlan_info.vlan_oid = vlan_oid;
    vlan_info.name = name.empty() ? "VLAN_" + std::to_string(vlan_id) : name;
    vlan_info.created_at = getCurrentTimestamp();
    vlan_info.status = VLANStatus::ACTIVE;
    
    vlans_[vlan_id] = vlan_info;
    
    std::cout << "VLAN " << vlan_id << " (" << vlan_info.name << ") created successfully" << std::endl;
    return true;
}

bool SAIVLANManager::deleteVLAN(uint16_t vlan_id) {
    if (!initialized_) {
        std::cerr << "SAI not initialized" << std::endl;
        return false;
    }
    
    auto it = vlans_.find(vlan_id);
    if (it == vlans_.end()) {
        std::cerr << "VLAN " << vlan_id << " not found" << std::endl;
        return false;
    }
    
    // Remove all port members first
    for (const auto& member : it->second.members) {
        removePortFromVLAN(vlan_id, member.port_name);
    }
    
    // Delete VLAN from SAI
    sai_status_t status = sai_adapter_->getVLANAPI()->remove_vlan(it->second.vlan_oid);
    if (status != SAI_STATUS_SUCCESS) {
        std::cerr << "Failed to delete VLAN " << vlan_id << ": " << status << std::endl;
        return false;
    }
    
    vlans_.erase(it);
    std::cout << "VLAN " << vlan_id << " deleted successfully" << std::endl;
    return true;
}

bool SAIVLANManager::addPortToVLAN(uint16_t vlan_id, const std::string& port_name, bool tagged) {
    if (!initialized_) {
        std::cerr << "SAI not initialized" << std::endl;
        return false;
    }
    
    auto vlan_it = vlans_.find(vlan_id);
    if (vlan_it == vlans_.end()) {
        std::cerr << "VLAN " << vlan_id << " not found" << std::endl;
        return false;
    }
    
    // Get port OID (in real implementation, this would query the port database)
    sai_object_id_t port_oid = getPortOID(port_name);
    if (port_oid == SAI_NULL_OBJECT_ID) {
        std::cerr << "Port " << port_name << " not found" << std::endl;
        return false;
    }
    
    // Create VLAN member
    sai_attribute_t vlan_member_attrs[3];
    
    vlan_member_attrs[0].id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
    vlan_member_attrs[0].value.oid = vlan_it->second.vlan_oid;
    
    vlan_member_attrs[1].id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
    vlan_member_attrs[1].value.oid = port_oid;
    
    vlan_member_attrs[2].id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;
    vlan_member_attrs[2].value.s32 = tagged ? SAI_VLAN_TAGGING_MODE_TAGGED : SAI_VLAN_TAGGING_MODE_UNTAGGED;
    
    sai_object_id_t vlan_member_oid;
    sai_status_t status = sai_adapter_->getVLANAPI()->create_vlan_member(&vlan_member_oid, sai_adapter_->getSwitchId(), 3, vlan_member_attrs);
    
    if (status != SAI_STATUS_SUCCESS) {
        std::cerr << "Failed to add port " << port_name << " to VLAN " << vlan_id << ": " << status << std::endl;
        return false;
    }
    
    // Store member information
    VLANMember member;
    member.port_name = port_name;
    member.port_oid = port_oid;
    member.member_oid = vlan_member_oid;
    member.tagged = tagged;
    member.added_at = getCurrentTimestamp();
    
    vlan_it->second.members.push_back(member);
    
    std::string tag_type = tagged ? "tagged" : "untagged";
    std::cout << "Port " << port_name << " added to VLAN " << vlan_id << " as " << tag_type << std::endl;
    return true;
}

bool SAIVLANManager::removePortFromVLAN(uint16_t vlan_id, const std::string& port_name) {
    if (!initialized_) {
        std::cerr << "SAI not initialized" << std::endl;
        return false;
    }
    
    auto vlan_it = vlans_.find(vlan_id);
    if (vlan_it == vlans_.end()) {
        std::cerr << "VLAN " << vlan_id << " not found" << std::endl;
        return false;
    }
    
    // Find the member
    auto member_it = std::find_if(vlan_it->second.members.begin(), vlan_it->second.members.end(),
        [&port_name](const VLANMember& member) {
            return member.port_name == port_name;
        });
    
    if (member_it == vlan_it->second.members.end()) {
        std::cerr << "Port " << port_name << " not found in VLAN " << vlan_id << std::endl;
        return false;
    }
    
    // Remove VLAN member from SAI
    sai_status_t status = sai_adapter_->getVLANAPI()->remove_vlan_member(member_it->member_oid);
    if (status != SAI_STATUS_SUCCESS) {
        std::cerr << "Failed to remove port " << port_name << " from VLAN " << vlan_id << ": " << status << std::endl;
        return false;
    }
    
    vlan_it->second.members.erase(member_it);
    std::cout << "Port " << port_name << " removed from VLAN " << vlan_id << std::endl;
    return true;
}

bool SAIVLANManager::validateVLANIsolation(uint16_t vlan1_id, uint16_t vlan2_id) {
    auto vlan1_it = vlans_.find(vlan1_id);
    auto vlan2_it = vlans_.find(vlan2_id);
    
    if (vlan1_it == vlans_.end() || vlan2_it == vlans_.end()) {
        std::cerr << "One or both VLANs not found" << std::endl;
        return false;
    }
    
    // Check for untagged port overlap (which would break isolation)
    std::set<std::string> vlan1_untagged, vlan2_untagged;
    
    for (const auto& member : vlan1_it->second.members) {
        if (!member.tagged) {
            vlan1_untagged.insert(member.port_name);
        }
    }
    
    for (const auto& member : vlan2_it->second.members) {
        if (!member.tagged) {
            vlan2_untagged.insert(member.port_name);
        }
    }
    
    // Find intersection
    std::vector<std::string> overlap;
    std::set_intersection(vlan1_untagged.begin(), vlan1_untagged.end(),
                         vlan2_untagged.begin(), vlan2_untagged.end(),
                         std::back_inserter(overlap));
    
    if (!overlap.empty()) {
        std::cerr << "VLAN isolation violation: Ports ";
        for (const auto& port : overlap) {
            std::cerr << port << " ";
        }
        std::cerr << "are untagged in both VLANs " << vlan1_id << " and " << vlan2_id << std::endl;
        return false;
    }
    
    std::cout << "VLAN isolation validated between VLAN " << vlan1_id << " and VLAN " << vlan2_id << std::endl;
    return true;
}

std::vector<VLANInfo> SAIVLANManager::getAllVLANs() const {
    std::vector<VLANInfo> result;
    for (const auto& pair : vlans_) {
        result.push_back(pair.second);
    }
    return result;
}

VLANInfo SAIVLANManager::getVLANInfo(uint16_t vlan_id) const {
    auto it = vlans_.find(vlan_id);
    if (it != vlans_.end()) {
        return it->second;
    }
    return VLANInfo{}; // Return empty struct if not found
}

void SAIVLANManager::printVLANStatus() const {
    std::cout << "\n=== VLAN Status ===" << std::endl;
    std::cout << std::setw(8) << "VLAN ID" 
              << std::setw(15) << "Name" 
              << std::setw(10) << "Status" 
              << std::setw(8) << "Members" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    for (const auto& pair : vlans_) {
        const auto& vlan = pair.second;
        std::cout << std::setw(8) << vlan.vlan_id
                  << std::setw(15) << vlan.name
                  << std::setw(10) << (vlan.status == VLANStatus::ACTIVE ? "Active" : "Inactive")
                  << std::setw(8) << vlan.members.size() << std::endl;
        
        for (const auto& member : vlan.members) {
            std::cout << "    " << member.port_name 
                      << " (" << (member.tagged ? "tagged" : "untagged") << ")" << std::endl;
        }
    }
}

sai_object_id_t SAIVLANManager::getPortOID(const std::string& port_name) {
    // In a real implementation, this would query the port database
    // For simulation, we'll generate a mock OID based on port name
    static std::map<std::string, sai_object_id_t> port_oids;
    
    if (port_oids.find(port_name) == port_oids.end()) {
        // Generate a mock OID (in real implementation, this would come from SAI)
        port_oids[port_name] = 0x1000000000000000ULL + port_oids.size();
    }
    
    return port_oids[port_name];
}

std::string SAIVLANManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void SAIVLANManager::cleanup() {
    if (initialized_) {
        // Clean up all VLANs
        for (auto& pair : vlans_) {
            deleteVLAN(pair.first);
        }
        
        // Uninitialize SAI
        sai_api_uninitialize();
        initialized_ = false;
        std::cout << "SAI VLAN Manager cleaned up" << std::endl;
    }
}

} // namespace sai
} // namespace sonic
