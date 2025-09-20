/**
 * @file sai_demo.cpp
 * @brief SAI Demo Application
 */

#include "../sai/sai_vlan_manager.h"
#include <iostream>

using namespace sonic::sai;

int main() {
    std::cout << "SONiC SAI Demo Starting..." << std::endl;

    SAIVLANManager vlan_manager;

    // Create demo VLANs
    if (vlan_manager.createVLAN(100, "Demo_VLAN_100")) {
        std::cout << "Created VLAN 100" << std::endl;
    }

    if (vlan_manager.createVLAN(200, "Demo_VLAN_200")) {
        std::cout << "Created VLAN 200" << std::endl;
    }

    // List VLANs
    auto vlans = vlan_manager.getAllVLANs();
    std::cout << "Total VLANs: " << vlans.size() << std::endl;

    std::cout << "SAI Demo completed successfully" << std::endl;
    return 0;
}