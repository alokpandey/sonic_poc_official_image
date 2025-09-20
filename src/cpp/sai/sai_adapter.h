/**
 * @file sai_adapter.h
 * @brief SAI Adapter Header - Handles both real SAI and mock SAI implementations
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#ifndef SONIC_SAI_ADAPTER_H
#define SONIC_SAI_ADAPTER_H

#include <mutex>

// Real SAI headers
extern "C" {
#include "sai.h"
#include "saivlan.h"
#include "sairoute.h"
#include "saiport.h"
#include "saiswitch.h"
#include "saibridge.h"
}

namespace sonic {
namespace sai {

/**
 * @brief SAI Adapter class - Singleton pattern
 * 
 * This class provides a unified interface to both real SAI and mock SAI implementations.
 * It automatically detects the available SAI environment and initializes accordingly.
 */
class SAIAdapter {
public:
    /**
     * @brief Get singleton instance
     * @return SAI Adapter instance
     */
    static SAIAdapter* getInstance();
    
    /**
     * @brief Initialize SAI
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Get VLAN API
     * @return VLAN API pointer
     */
    sai_vlan_api_t* getVLANAPI();
    
    /**
     * @brief Get Port API
     * @return Port API pointer
     */
    sai_port_api_t* getPortAPI();
    
    /**
     * @brief Get Route API
     * @return Route API pointer
     */
    sai_route_api_t* getRouteAPI();
    
    /**
     * @brief Get Bridge API
     * @return Bridge API pointer
     */
    sai_bridge_api_t* getBridgeAPI();
    
    /**
     * @brief Get Switch API
     * @return Switch API pointer
     */
    sai_switch_api_t* getSwitchAPI();
    
    /**
     * @brief Get Switch ID
     * @return Switch object ID
     */
    sai_object_id_t getSwitchId();
    
    /**
     * @brief Check if SAI is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;
    
    /**
     * @brief Check if using mock SAI
     * @return true if using mock, false if using real SAI
     */
    bool isUsingMock() const;

private:
    /**
     * @brief Private constructor (singleton)
     */
    SAIAdapter();
    
    /**
     * @brief Destructor
     */
    ~SAIAdapter();
    
    /**
     * @brief Detect SAI environment
     * @return true if detection successful
     */
    bool detectSAIEnvironment();
    
    /**
     * @brief Create switch instance
     * @return true if successful
     */
    bool createSwitchInstance();
    
    // Singleton instance
    static SAIAdapter* instance_;
    static std::mutex instance_mutex_;
    
    // SAI state
    bool initialized_;
    bool use_mock_;
    sai_object_id_t switch_id_;
    
    // SAI API pointers
    sai_switch_api_t* switch_api_;
    sai_vlan_api_t* vlan_api_;
    sai_port_api_t* port_api_;
    sai_route_api_t* route_api_;
    sai_bridge_api_t* bridge_api_;
    
    // Disable copy constructor and assignment operator
    SAIAdapter(const SAIAdapter&) = delete;
    SAIAdapter& operator=(const SAIAdapter&) = delete;
};

} // namespace sai
} // namespace sonic

#endif // SONIC_SAI_ADAPTER_H
