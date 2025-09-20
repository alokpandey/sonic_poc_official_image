/**
 * @file mock_sai.h
 * @brief Mock SAI Header for SONiC POC
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#ifndef MOCK_SAI_H
#define MOCK_SAI_H

#include <stdint.h>
#include <string>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif

// SAI Status Codes
typedef enum _sai_status_t {
    SAI_STATUS_SUCCESS = 0,
    SAI_STATUS_FAILURE = -1,
    SAI_STATUS_NOT_SUPPORTED = -2,
    SAI_STATUS_NO_MEMORY = -3,
    SAI_STATUS_INSUFFICIENT_RESOURCES = -4,
    SAI_STATUS_INVALID_PARAMETER = -5,
    SAI_STATUS_ITEM_NOT_FOUND = -6,
    SAI_STATUS_BUFFER_OVERFLOW = -7,
    SAI_STATUS_INVALID_PORT_NUMBER = -8,
    SAI_STATUS_INVALID_PORT_MEMBER = -9,
    SAI_STATUS_INVALID_VLAN_ID = -10,
    SAI_STATUS_UNINITIALIZED = -11,
    SAI_STATUS_TABLE_FULL = -12,
    SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING = -13,
    SAI_STATUS_NOT_IMPLEMENTED = -14,
    SAI_STATUS_ADDR_NOT_FOUND = -15
} sai_status_t;

// SAI Object Types
typedef enum _sai_object_type_t {
    SAI_OBJECT_TYPE_NULL = 0,
    SAI_OBJECT_TYPE_PORT = 1,
    SAI_OBJECT_TYPE_LAG = 2,
    SAI_OBJECT_TYPE_VIRTUAL_ROUTER = 3,
    SAI_OBJECT_TYPE_NEXT_HOP = 4,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP = 5,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE = 6,
    SAI_OBJECT_TYPE_ACL_TABLE = 7,
    SAI_OBJECT_TYPE_ACL_ENTRY = 8,
    SAI_OBJECT_TYPE_ACL_COUNTER = 9,
    SAI_OBJECT_TYPE_HOST_INTERFACE = 10,
    SAI_OBJECT_TYPE_MIRROR_SESSION = 11,
    SAI_OBJECT_TYPE_SAMPLEPACKET = 12,
    SAI_OBJECT_TYPE_STP_INSTANCE = 13,
    SAI_OBJECT_TYPE_MAX_PORTS = 14,
    SAI_OBJECT_TYPE_VLAN = 15,
    SAI_OBJECT_TYPE_VLAN_MEMBER = 16,
    SAI_OBJECT_TYPE_FDB_ENTRY = 17,
    SAI_OBJECT_TYPE_SWITCH = 18,
    SAI_OBJECT_TYPE_HOSTIF_TRAP = 19,
    SAI_OBJECT_TYPE_HOSTIF_TABLE_ENTRY = 20,
    SAI_OBJECT_TYPE_NEIGHBOR_ENTRY = 21,
    SAI_OBJECT_TYPE_ROUTE_ENTRY = 22,
    SAI_OBJECT_TYPE_QOS_MAPS = 23,
    SAI_OBJECT_TYPE_QUEUE = 24,
    SAI_OBJECT_TYPE_SCHEDULER = 25,
    SAI_OBJECT_TYPE_SCHEDULER_GROUP = 26,
    SAI_OBJECT_TYPE_BUFFER_POOL = 27,
    SAI_OBJECT_TYPE_BUFFER_PROFILE = 28,
    SAI_OBJECT_TYPE_POLICER = 29,
    SAI_OBJECT_TYPE_WRED = 30,
    SAI_OBJECT_TYPE_QOS_MAP = 31,
    SAI_OBJECT_TYPE_BRIDGE = 32,
    SAI_OBJECT_TYPE_BRIDGE_PORT = 33
} sai_object_type_t;

// SAI API Types
typedef enum _sai_api_t {
    SAI_API_UNSPECIFIED = 0,
    SAI_API_SWITCH = 1,
    SAI_API_PORT = 2,
    SAI_API_FDB = 3,
    SAI_API_VLAN = 4,
    SAI_API_VIRTUAL_ROUTER = 5,
    SAI_API_ROUTE = 6,
    SAI_API_NEXT_HOP = 7,
    SAI_API_NEXT_HOP_GROUP = 8,
    SAI_API_ROUTER_INTERFACE = 9,
    SAI_API_NEIGHBOR = 10,
    SAI_API_ACL = 11,
    SAI_API_HOST_INTERFACE = 12,
    SAI_API_MIRROR = 13,
    SAI_API_SAMPLEPACKET = 14,
    SAI_API_STP = 15,
    SAI_API_LAG = 16,
    SAI_API_POLICER = 17,
    SAI_API_WRED = 18,
    SAI_API_QOS_MAPS = 19,
    SAI_API_QUEUE = 20,
    SAI_API_SCHEDULER = 21,
    SAI_API_SCHEDULER_GROUP = 22,
    SAI_API_BUFFERS = 23,
    SAI_API_QOS_MAP = 24,
    SAI_API_HOSTIF_TRAP = 25,
    SAI_API_BRIDGE = 33,
    SAI_API_MAX = 34
} sai_api_t;

// Basic types
typedef uint64_t sai_object_id_t;
typedef uint16_t sai_vlan_id_t;
typedef uint32_t sai_ip4_t;

#define SAI_NULL_OBJECT_ID 0ULL

// SAI Attribute Value Union
typedef union _sai_attribute_value_t {
    bool booldata;
    char chardata;
    uint8_t u8;
    int8_t s8;
    uint16_t u16;
    int16_t s16;
    uint32_t u32;
    int32_t s32;
    uint64_t u64;
    int64_t s64;
    sai_object_id_t oid;
    // Add more types as needed
} sai_attribute_value_t;

// SAI Attribute
typedef struct _sai_attribute_t {
    int32_t id;
    sai_attribute_value_t value;
} sai_attribute_t;

// VLAN Attributes
typedef enum _sai_vlan_attr_t {
    SAI_VLAN_ATTR_START = 0,
    SAI_VLAN_ATTR_VLAN_ID = SAI_VLAN_ATTR_START,
    SAI_VLAN_ATTR_MEMBER_LIST = 1,
    SAI_VLAN_ATTR_MAX_LEARNED_ADDRESSES = 2,
    SAI_VLAN_ATTR_STP_INSTANCE = 3,
    SAI_VLAN_ATTR_LEARN_DISABLE = 4,
    SAI_VLAN_ATTR_IPV4_MCAST_LOOKUP_KEY_TYPE = 5,
    SAI_VLAN_ATTR_IPV6_MCAST_LOOKUP_KEY_TYPE = 6,
    SAI_VLAN_ATTR_UNKNOWN_NON_IP_MCAST_OUTPUT_GROUP_ID = 7,
    SAI_VLAN_ATTR_UNKNOWN_IPV4_MCAST_OUTPUT_GROUP_ID = 8,
    SAI_VLAN_ATTR_UNKNOWN_IPV6_MCAST_OUTPUT_GROUP_ID = 9,
    SAI_VLAN_ATTR_UNKNOWN_LINKLOCAL_MCAST_OUTPUT_GROUP_ID = 10,
    SAI_VLAN_ATTR_INGRESS_ACL = 11,
    SAI_VLAN_ATTR_EGRESS_ACL = 12,
    SAI_VLAN_ATTR_META_DATA = 13,
    SAI_VLAN_ATTR_END = 14
} sai_vlan_attr_t;

// VLAN Member Attributes
typedef enum _sai_vlan_member_attr_t {
    SAI_VLAN_MEMBER_ATTR_START = 0,
    SAI_VLAN_MEMBER_ATTR_VLAN_ID = SAI_VLAN_MEMBER_ATTR_START,
    SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID = 1,
    SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE = 2,
    SAI_VLAN_MEMBER_ATTR_END = 3
} sai_vlan_member_attr_t;

// VLAN Tagging Mode
typedef enum _sai_vlan_tagging_mode_t {
    SAI_VLAN_TAGGING_MODE_UNTAGGED = 0,
    SAI_VLAN_TAGGING_MODE_TAGGED = 1,
    SAI_VLAN_TAGGING_MODE_PRIORITY_TAGGED = 2
} sai_vlan_tagging_mode_t;

// Route Entry Attributes
typedef enum _sai_route_entry_attr_t {
    SAI_ROUTE_ENTRY_ATTR_START = 0,
    SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION = SAI_ROUTE_ENTRY_ATTR_START,
    SAI_ROUTE_ENTRY_ATTR_USER_TRAP_ID = 1,
    SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID = 2,
    SAI_ROUTE_ENTRY_ATTR_META_DATA = 3,
    SAI_ROUTE_ENTRY_ATTR_END = 4
} sai_route_entry_attr_t;

// Packet Action
typedef enum _sai_packet_action_t {
    SAI_PACKET_ACTION_DROP = 0,
    SAI_PACKET_ACTION_FORWARD = 1,
    SAI_PACKET_ACTION_COPY = 2,
    SAI_PACKET_ACTION_COPY_CANCEL = 3,
    SAI_PACKET_ACTION_TRAP = 4,
    SAI_PACKET_ACTION_LOG = 5,
    SAI_PACKET_ACTION_DENY = 6,
    SAI_PACKET_ACTION_TRANSIT = 7
} sai_packet_action_t;

// Route Entry
typedef struct _sai_route_entry_t {
    sai_object_id_t switch_id;
    sai_object_id_t vr_id;
    // Simplified - in real SAI this would include destination prefix
} sai_route_entry_t;

// Service Method Table (simplified)
typedef struct _sai_service_method_table_t {
    // Placeholder for service methods
    void* reserved;
} sai_service_method_table_t;

// Switch Attributes
typedef enum _sai_switch_attr_t {
    SAI_SWITCH_ATTR_START = 0,
    SAI_SWITCH_ATTR_INIT_SWITCH = SAI_SWITCH_ATTR_START,
    SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY = 1,
    SAI_SWITCH_ATTR_END = 2
} sai_switch_attr_t;

// Forward declarations for API structures
typedef struct _sai_vlan_api_t sai_vlan_api_t;
typedef struct _sai_route_api_t sai_route_api_t;
typedef struct _sai_port_api_t sai_port_api_t;
typedef struct _sai_switch_api_t sai_switch_api_t;
typedef struct _sai_bridge_api_t sai_bridge_api_t;

// API function pointers
typedef sai_status_t (*sai_create_vlan_fn)(sai_object_id_t* vlan_id, sai_object_id_t switch_id,
                                           uint32_t attr_count, const sai_attribute_t* attr_list);
typedef sai_status_t (*sai_remove_vlan_fn)(sai_object_id_t vlan_id);

// Switch API function pointers
typedef sai_status_t (*sai_create_switch_fn)(sai_object_id_t* switch_id,
                                             uint32_t attr_count, const sai_attribute_t* attr_list);
typedef sai_status_t (*sai_remove_switch_fn)(sai_object_id_t switch_id);

// Bridge API function pointers
typedef sai_status_t (*sai_create_bridge_fn)(sai_object_id_t* bridge_id, sai_object_id_t switch_id,
                                             uint32_t attr_count, const sai_attribute_t* attr_list);
typedef sai_status_t (*sai_remove_bridge_fn)(sai_object_id_t bridge_id);
typedef sai_status_t (*sai_create_vlan_member_fn)(sai_object_id_t* vlan_member_id, sai_object_id_t switch_id,
                                                  uint32_t attr_count, const sai_attribute_t* attr_list);
typedef sai_status_t (*sai_remove_vlan_member_fn)(sai_object_id_t vlan_member_id);

typedef sai_status_t (*sai_create_route_entry_fn)(const sai_route_entry_t* route_entry, uint32_t attr_count,
                                                   const sai_attribute_t* attr_list);
typedef sai_status_t (*sai_remove_route_entry_fn)(const sai_route_entry_t* route_entry);

// API Structures
struct _sai_vlan_api_t {
    sai_create_vlan_fn create_vlan;
    sai_remove_vlan_fn remove_vlan;
    sai_create_vlan_member_fn create_vlan_member;
    sai_remove_vlan_member_fn remove_vlan_member;
    // Add more function pointers as needed
};

struct _sai_route_api_t {
    sai_create_route_entry_fn create_route_entry;
    sai_remove_route_entry_fn remove_route_entry;
    // Add more function pointers as needed
};

struct _sai_port_api_t {
    // Port API functions (placeholder)
    void* reserved;
};

struct _sai_switch_api_t {
    sai_create_switch_fn create_switch;
    sai_remove_switch_fn remove_switch;
};

struct _sai_bridge_api_t {
    sai_create_bridge_fn create_bridge;
    sai_remove_bridge_fn remove_bridge;
};

// Core SAI functions
sai_status_t sai_api_initialize(uint64_t flags, const sai_service_method_table_t* services);
sai_status_t sai_api_uninitialize(void);
sai_status_t sai_api_query(sai_api_t api, void** api_method_table);

#ifdef __cplusplus
}

// C++ Mock SAI Object (for internal use)
struct MockSAIObject {
    sai_object_type_t type;
    sai_object_id_t switch_id;
    std::map<std::string, std::string> attributes;
};

// Mock API type definitions
typedef struct _sai_vlan_api_t MockVLANAPI;
typedef struct _sai_route_api_t MockRouteAPI;
typedef struct _sai_port_api_t MockPortAPI;
typedef struct _sai_switch_api_t MockSwitchAPI;
typedef struct _sai_bridge_api_t MockBridgeAPI;

#endif // __cplusplus

#endif // MOCK_SAI_H
