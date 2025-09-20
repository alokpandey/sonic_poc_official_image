#!/usr/bin/env python3
"""
Mock SONiC CLI
Simulates SONiC CLI commands for demonstration purposes
"""

import sys
import json
import os
import redis
from datetime import datetime

# Connect to Redis (SONiC database)
try:
    r = redis.Redis(host='localhost', port=6379, db=4, decode_responses=True)  # CONFIG_DB
except:
    r = None

def show_version():
    """Mock show version command"""
    return """SONiC Software Version: SONiC.202305.01
Distribution: Debian 11.7
Kernel: 5.10.0-23-2-amd64
Build commit: 12345678
Build date: Mon 01 May 2023 12:00:00 UTC
Built by: sonic-team@sonic.net

Platform: x86_64-kvm_x86_64-r0
HwSKU: Force10-S6000
ASIC: vs
Serial Number: N/A
Model Number: N/A
Hardware Version: N/A
Uptime: 01:23:45 up 1 day, 2:34, 1 user, load average: 0.12, 0.34, 0.56"""

def show_vlan_brief():
    """Mock show vlan brief command"""
    if r:
        try:
            # Get VLANs from Redis CONFIG_DB
            vlan_keys = r.keys("VLAN|*")
            output = "VLAN ID    Name             Status    Ports\n"
            output += "-------    ----             ------    -----\n"
            
            for key in vlan_keys:
                vlan_id = key.split('|')[1].replace('Vlan', '')
                vlan_data = r.hgetall(key)
                
                # Get VLAN members
                member_keys = r.keys(f"VLAN_MEMBER|Vlan{vlan_id}|*")
                ports = []
                for member_key in member_keys:
                    port = member_key.split('|')[-1]
                    ports.append(port)
                
                ports_str = ','.join(ports) if ports else "None"
                name = vlan_data.get('description', f'VLAN{vlan_id}')
                
                output += f"{vlan_id:<10} {name:<16} Active    {ports_str}\n"
            
            return output if vlan_keys else "No VLANs configured"
        except:
            pass
    
    # Fallback static output
    return """VLAN ID    Name             Status    Ports
-------    ----             ------    -----
100        Engineering      Active    Ethernet0,Ethernet4
200        Sales            Active    Ethernet8,Ethernet12
300        Management       Active    Ethernet16,Ethernet20"""

def show_interfaces_status():
    """Mock show interfaces status command"""
    return """Interface    Lanes    Speed    MTU    Alias    Vlan    Oper    Admin    Type    Asym PFC
---------    -----    -----    ---    -----    ----    ----    -----    ----    --------
Ethernet0    0        25G      9100   Eth1/1   routed  up      up       N/A     N/A
Ethernet4    1        25G      9100   Eth1/2   routed  up      up       N/A     N/A
Ethernet8    2        25G      9100   Eth1/3   routed  up      up       N/A     N/A
Ethernet12   3        25G      9100   Eth1/4   routed  up      up       N/A     N/A
Ethernet16   4        25G      9100   Eth1/5   routed  up      up       N/A     N/A
Ethernet20   5        25G      9100   Eth1/6   routed  up      up       N/A     N/A"""

def show_ip_interfaces():
    """Mock show ip interfaces command"""
    return """Interface    Master    IPv4 address/mask    Admin/Oper    BGP Neighbor    Neighbor IP
---------    ------    -----------------    ----------    ------------    -----------
Ethernet0    N/A       192.168.1.1/24      up/up         N/A             N/A
Ethernet4    N/A       192.168.2.1/24      up/up         N/A             N/A
lo           N/A       127.0.0.1/8          up/up         N/A             N/A"""

def show_mac():
    """Mock show mac command"""
    return """  Vlan    Mac Address         Type        Port
------  -----------------  --------  -----------
   100  52:54:00:12:34:01   Dynamic   Ethernet0
   100  52:54:00:12:34:02   Dynamic   Ethernet4
   200  52:54:00:12:34:03   Dynamic   Ethernet8
   200  52:54:00:12:34:04   Dynamic   Ethernet12
   300  52:54:00:12:34:05   Dynamic   Ethernet16
   300  52:54:00:12:34:06   Dynamic   Ethernet20
Total number of entries 6"""

def config_vlan_add(vlan_id):
    """Mock config vlan add command"""
    if r:
        try:
            vlan_key = f"VLAN|Vlan{vlan_id}"
            r.hset(vlan_key, "vlanid", vlan_id)
            print(f"VLAN {vlan_id} added successfully")
            return
        except:
            pass
    print(f"VLAN {vlan_id} added successfully")

def config_vlan_del(vlan_id):
    """Mock config vlan del command"""
    if r:
        try:
            vlan_key = f"VLAN|Vlan{vlan_id}"
            r.delete(vlan_key)
            # Also delete members
            member_keys = r.keys(f"VLAN_MEMBER|Vlan{vlan_id}|*")
            for key in member_keys:
                r.delete(key)
            print(f"VLAN {vlan_id} deleted successfully")
            return
        except:
            pass
    print(f"VLAN {vlan_id} deleted successfully")

def config_vlan_member_add(vlan_id, interface, tagged=False):
    """Mock config vlan member add command"""
    if r:
        try:
            member_key = f"VLAN_MEMBER|Vlan{vlan_id}|{interface}"
            tagging_mode = "tagged" if tagged else "untagged"
            r.hset(member_key, "tagging_mode", tagging_mode)
            print(f"Interface {interface} added to VLAN {vlan_id} as {tagging_mode}")
            return
        except:
            pass
    tag_str = "tagged" if tagged else "untagged"
    print(f"Interface {interface} added to VLAN {vlan_id} as {tag_str}")

def config_vlan_member_del(vlan_id, interface):
    """Mock config vlan member del command"""
    if r:
        try:
            member_key = f"VLAN_MEMBER|Vlan{vlan_id}|{interface}"
            r.delete(member_key)
            print(f"Interface {interface} removed from VLAN {vlan_id}")
            return
        except:
            pass
    print(f"Interface {interface} removed from VLAN {vlan_id}")

def sonic_cfggen():
    """Mock sonic-cfggen command"""
    config = {
        "DEVICE_METADATA": {
            "localhost": {
                "hostname": "sonic-vs",
                "platform": "x86_64-kvm_x86_64-r0",
                "mac": "52:54:00:12:34:56"
            }
        }
    }
    return json.dumps(config, indent=2)

def main():
    if len(sys.argv) < 2:
        print("Usage: mock-sonic-cli.py <command> [args...]")
        sys.exit(1)
    
    command = sys.argv[1]
    args = sys.argv[2:]
    
    # Determine which command was called based on script name or first argument
    script_name = os.path.basename(sys.argv[0])
    
    if script_name == "show" or command == "show":
        if len(args) == 0:
            print("Usage: show <subcommand>")
            sys.exit(1)
        
        subcommand = args[0]
        if subcommand == "version":
            print(show_version())
        elif subcommand == "vlan" and len(args) > 1 and args[1] == "brief":
            print(show_vlan_brief())
        elif subcommand == "interfaces" and len(args) > 1 and args[1] == "status":
            print(show_interfaces_status())
        elif subcommand == "ip" and len(args) > 1 and args[1] == "interfaces":
            print(show_ip_interfaces())
        elif subcommand == "mac":
            print(show_mac())
        else:
            print(f"Unknown show command: {' '.join(args)}")
    
    elif script_name == "config" or command == "config":
        if len(args) < 2:
            print("Usage: config <subcommand> <action> [args...]")
            sys.exit(1)
        
        subcommand = args[0]
        action = args[1]
        
        if subcommand == "vlan":
            if action == "add" and len(args) > 2:
                config_vlan_add(args[2])
            elif action == "del" and len(args) > 2:
                config_vlan_del(args[2])
            elif action == "member":
                if len(args) > 4 and args[2] == "add":
                    vlan_id = args[3]
                    interface = args[4]
                    tagged = "--tagged" in args
                    config_vlan_member_add(vlan_id, interface, tagged)
                elif len(args) > 4 and args[2] == "del":
                    vlan_id = args[3]
                    interface = args[4]
                    config_vlan_member_del(vlan_id, interface)
        else:
            print(f"Configuration applied: {' '.join(args)}")
    
    elif script_name == "sonic-cfggen" or command == "sonic-cfggen":
        if "--print-data" in args:
            print(sonic_cfggen())
        else:
            print("SONiC configuration generator")
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

if __name__ == "__main__":
    main()
