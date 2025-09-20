/**
 * @file mock_sai.cpp
 * @brief Mock SAI Implementation for SONiC POC
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#include "mock_sai.h"
#include <iostream>
#include <map>
#include <vector>
#include <mutex>

// Global mock SAI state
static std::mutex g_sai_mutex;
static bool g_sai_initialized = false;
static std::map<sai_api_t, void*> g_api_table;
static std::map<sai_object_id_t, MockSAIObject> g_objects;
static sai_object_id_t g_next_oid = 0x1000000000000000ULL;

// Mock API implementations
static MockVLANAPI g_vlan_api;
static MockRouteAPI g_route_api;
static MockPortAPI g_port_api;
static MockSwitchAPI g_switch_api;
static MockBridgeAPI g_bridge_api;

/**
 * @brief Generate next object ID
 */
static sai_object_id_t generateNextOID() {
    return ++g_next_oid;
}

/**
 * @brief SAI API Initialize
 */
sai_status_t sai_api_initialize(uint64_t flags, const sai_service_method_table_t* services) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    (void)flags;    // Unused parameter
    (void)services; // Unused parameter
    
    if (g_sai_initialized) {
        return SAI_STATUS_SUCCESS;
    }
    
    // Initialize API table
    g_api_table[SAI_API_VLAN] = &g_vlan_api;
    g_api_table[SAI_API_ROUTE] = &g_route_api;
    g_api_table[SAI_API_PORT] = &g_port_api;
    g_api_table[SAI_API_SWITCH] = &g_switch_api;
    g_api_table[SAI_API_BRIDGE] = &g_bridge_api;
    
    g_sai_initialized = true;
    std::cout << "Mock SAI initialized successfully" << std::endl;
    
    return SAI_STATUS_SUCCESS;
}

/**
 * @brief SAI API Uninitialize
 */
sai_status_t sai_api_uninitialize() {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    if (!g_sai_initialized) {
        return SAI_STATUS_SUCCESS;
    }
    
    // Clear all objects
    g_objects.clear();
    g_api_table.clear();
    g_next_oid = 0x1000000000000000ULL;
    
    g_sai_initialized = false;
    std::cout << "Mock SAI uninitialized" << std::endl;
    
    return SAI_STATUS_SUCCESS;
}

/**
 * @brief SAI API Query
 */
sai_status_t sai_api_query(sai_api_t api, void** api_method_table) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);

    std::cout << "sai_api_query called for API: " << api << std::endl;

    if (!g_sai_initialized) {
        std::cout << "SAI not initialized" << std::endl;
        return SAI_STATUS_UNINITIALIZED;
    }

    if (!api_method_table) {
        std::cout << "Invalid parameter: api_method_table is null" << std::endl;
        return SAI_STATUS_INVALID_PARAMETER;
    }

    auto it = g_api_table.find(api);
    if (it == g_api_table.end()) {
        std::cout << "API " << api << " not supported" << std::endl;
        return SAI_STATUS_NOT_SUPPORTED;
    }

    *api_method_table = it->second;
    std::cout << "API " << api << " query successful" << std::endl;
    return SAI_STATUS_SUCCESS;
}

// Mock VLAN API Implementation
sai_status_t mock_create_vlan(sai_object_id_t* vlan_id, sai_object_id_t switch_id, 
                              uint32_t attr_count, const sai_attribute_t* attr_list) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    if (!vlan_id || !attr_list) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    
    // Generate new VLAN OID
    *vlan_id = generateNextOID();
    
    // Create mock object
    MockSAIObject obj;
    obj.type = SAI_OBJECT_TYPE_VLAN;
    obj.switch_id = switch_id;
    
    // Parse attributes
    for (uint32_t i = 0; i < attr_count; i++) {
        if (attr_list[i].id == SAI_VLAN_ATTR_VLAN_ID) {
            obj.attributes["vlan_id"] = std::to_string(attr_list[i].value.u16);
        }
    }
    
    g_objects[*vlan_id] = obj;
    
    std::cout << "Mock: Created VLAN with OID " << std::hex << *vlan_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_remove_vlan(sai_object_id_t vlan_id) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    auto it = g_objects.find(vlan_id);
    if (it == g_objects.end()) {
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    
    g_objects.erase(it);
    
    std::cout << "Mock: Removed VLAN with OID " << std::hex << vlan_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_create_vlan_member(sai_object_id_t* vlan_member_id, sai_object_id_t switch_id,
                                     uint32_t attr_count, const sai_attribute_t* attr_list) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    if (!vlan_member_id || !attr_list) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    
    // Generate new VLAN member OID
    *vlan_member_id = generateNextOID();
    
    // Create mock object
    MockSAIObject obj;
    obj.type = SAI_OBJECT_TYPE_VLAN_MEMBER;
    obj.switch_id = switch_id;
    
    // Parse attributes
    for (uint32_t i = 0; i < attr_count; i++) {
        switch (attr_list[i].id) {
            case SAI_VLAN_MEMBER_ATTR_VLAN_ID:
                obj.attributes["vlan_id"] = std::to_string(attr_list[i].value.oid);
                break;
            case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID:
                obj.attributes["port_id"] = std::to_string(attr_list[i].value.oid);
                break;
            case SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE:
                obj.attributes["tagging_mode"] = std::to_string(attr_list[i].value.s32);
                break;
        }
    }
    
    g_objects[*vlan_member_id] = obj;
    
    std::cout << "Mock: Created VLAN member with OID " << std::hex << *vlan_member_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_remove_vlan_member(sai_object_id_t vlan_member_id) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    auto it = g_objects.find(vlan_member_id);
    if (it == g_objects.end()) {
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    
    g_objects.erase(it);
    
    std::cout << "Mock: Removed VLAN member with OID " << std::hex << vlan_member_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

// Mock Switch API Implementation
sai_status_t mock_create_switch(sai_object_id_t* switch_id,
                                uint32_t attr_count, const sai_attribute_t* attr_list) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);

    if (!switch_id) {
        return SAI_STATUS_INVALID_PARAMETER;
    }

    // Generate a switch ID
    *switch_id = generateNextOID();

    std::cout << "Mock: Created switch with ID: " << std::hex << *switch_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_remove_switch(sai_object_id_t switch_id) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);

    std::cout << "Mock: Removed switch with ID: " << std::hex << switch_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

// Mock Bridge API Implementation
sai_status_t mock_create_bridge(sai_object_id_t* bridge_id, sai_object_id_t switch_id,
                                uint32_t attr_count, const sai_attribute_t* attr_list) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);

    if (!bridge_id) {
        return SAI_STATUS_INVALID_PARAMETER;
    }

    // Generate a bridge ID
    *bridge_id = generateNextOID();

    std::cout << "Mock: Created bridge with ID: " << std::hex << *bridge_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_remove_bridge(sai_object_id_t bridge_id) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);

    std::cout << "Mock: Removed bridge with ID: " << std::hex << bridge_id << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

// Mock Route API Implementation
sai_status_t mock_create_route_entry(const sai_route_entry_t* route_entry, uint32_t attr_count,
                                     const sai_attribute_t* attr_list) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    if (!route_entry || !attr_list) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    
    // Create mock route object (using destination as key)
    sai_object_id_t route_oid = generateNextOID();
    
    MockSAIObject obj;
    obj.type = SAI_OBJECT_TYPE_ROUTE_ENTRY;
    obj.switch_id = route_entry->switch_id;
    
    // Parse attributes
    for (uint32_t i = 0; i < attr_count; i++) {
        switch (attr_list[i].id) {
            case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
                obj.attributes["packet_action"] = std::to_string(attr_list[i].value.s32);
                break;
            case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
                obj.attributes["next_hop_id"] = std::to_string(attr_list[i].value.oid);
                break;
        }
    }
    
    g_objects[route_oid] = obj;
    
    std::cout << "Mock: Created route entry with OID " << std::hex << route_oid << std::dec << std::endl;
    return SAI_STATUS_SUCCESS;
}

sai_status_t mock_remove_route_entry(const sai_route_entry_t* route_entry) {
    std::lock_guard<std::mutex> lock(g_sai_mutex);
    
    if (!route_entry) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    
    // In a real implementation, we would find the route by destination
    // For mock, we'll just simulate success
    
    std::cout << "Mock: Removed route entry" << std::endl;
    return SAI_STATUS_SUCCESS;
}

// Initialize API function pointers
void initializeMockAPIs() {
    // VLAN API
    g_vlan_api.create_vlan = mock_create_vlan;
    g_vlan_api.remove_vlan = mock_remove_vlan;
    g_vlan_api.create_vlan_member = mock_create_vlan_member;
    g_vlan_api.remove_vlan_member = mock_remove_vlan_member;
    
    // Route API
    g_route_api.create_route_entry = mock_create_route_entry;
    g_route_api.remove_route_entry = mock_remove_route_entry;

    // Switch API
    g_switch_api.create_switch = mock_create_switch;
    g_switch_api.remove_switch = mock_remove_switch;

    // Bridge API
    g_bridge_api.create_bridge = mock_create_bridge;
    g_bridge_api.remove_bridge = mock_remove_bridge;

    // Port API (minimal implementation)
}

// Static initialization
static bool g_apis_initialized = []() {
    initializeMockAPIs();
    return true;
}();
