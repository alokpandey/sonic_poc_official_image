/**
 * @file sai_adapter.cpp
 * @brief SAI Adapter - Handles both real SAI and mock SAI implementations
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#include "sai_adapter.h"
#include <iostream>
#include <dlfcn.h>

namespace sonic {
namespace sai {

SAIAdapter* SAIAdapter::instance_ = nullptr;
std::mutex SAIAdapter::instance_mutex_;

SAIAdapter* SAIAdapter::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = new SAIAdapter();
    }
    return instance_;
}

SAIAdapter::SAIAdapter() : initialized_(false), use_mock_(false) {
    // Try to detect if we're running with real SAI or mock
    detectSAIEnvironment();
}

SAIAdapter::~SAIAdapter() {
    if (initialized_) {
        sai_api_uninitialize();
    }
}

bool SAIAdapter::detectSAIEnvironment() {
    // Try to load real SAI library
    void* sai_lib = dlopen("libsai.so", RTLD_LAZY);
    if (sai_lib) {
        dlclose(sai_lib);
        use_mock_ = false;
        std::cout << "Real SAI library detected" << std::endl;
        return true;
    }
    
    // Fall back to mock SAI
    use_mock_ = true;
    std::cout << "Using Mock SAI implementation" << std::endl;
    return true;
}

bool SAIAdapter::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Initialize SAI API
        sai_status_t status = sai_api_initialize(0, nullptr);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to initialize SAI API: " << status << std::endl;
            return false;
        }
        
        // Query required APIs
        status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api_);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to query Switch API: " << status << std::endl;
            return false;
        }
        
        status = sai_api_query(SAI_API_VLAN, (void**)&vlan_api_);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to query VLAN API: " << status << std::endl;
            return false;
        }
        
        status = sai_api_query(SAI_API_PORT, (void**)&port_api_);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to query Port API: " << status << std::endl;
            return false;
        }
        
        status = sai_api_query(SAI_API_ROUTE, (void**)&route_api_);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to query Route API: " << status << std::endl;
            return false;
        }
        
        status = sai_api_query(SAI_API_BRIDGE, (void**)&bridge_api_);
        if (status != SAI_STATUS_SUCCESS) {
            std::cerr << "Failed to query Bridge API: " << status << std::endl;
            return false;
        }
        
        // Create switch instance (required for most SAI operations)
        if (!createSwitchInstance()) {
            std::cerr << "Failed to create switch instance" << std::endl;
            return false;
        }
        
        initialized_ = true;
        std::cout << "SAI Adapter initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during SAI initialization: " << e.what() << std::endl;
        return false;
    }
}

bool SAIAdapter::createSwitchInstance() {
    if (!switch_api_) {
        return false;
    }
    
    // Create switch with minimal attributes
    sai_attribute_t switch_attrs[1];
    
    // Set switch initialization mode
    switch_attrs[0].id = SAI_SWITCH_ATTR_INIT_SWITCH;
    switch_attrs[0].value.booldata = true;
    
    sai_status_t status = switch_api_->create_switch(&switch_id_, 1, switch_attrs);
    if (status != SAI_STATUS_SUCCESS) {
        std::cerr << "Failed to create switch instance: " << status << std::endl;
        return false;
    }
    
    std::cout << "Switch instance created with ID: " << std::hex << switch_id_ << std::dec << std::endl;
    return true;
}

sai_vlan_api_t* SAIAdapter::getVLANAPI() {
    return vlan_api_;
}

sai_port_api_t* SAIAdapter::getPortAPI() {
    return port_api_;
}

sai_route_api_t* SAIAdapter::getRouteAPI() {
    return route_api_;
}

sai_bridge_api_t* SAIAdapter::getBridgeAPI() {
    return bridge_api_;
}

sai_switch_api_t* SAIAdapter::getSwitchAPI() {
    return switch_api_;
}

sai_object_id_t SAIAdapter::getSwitchId() {
    return switch_id_;
}

bool SAIAdapter::isInitialized() const {
    return initialized_;
}

bool SAIAdapter::isUsingMock() const {
    return use_mock_;
}

} // namespace sai
} // namespace sonic
