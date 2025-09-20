#!/usr/bin/env python3
"""
Mock SONiC Orchagent
Simulates the SONiC orchestration agent
"""

import sys
import time
import redis
import json
import logging
from datetime import datetime

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('orchagent')

class MockOrchAgent:
    def __init__(self):
        self.redis_host = 'localhost'
        self.redis_port = 6379
        self.running = True
        
        # Connect to SONiC databases
        try:
            self.config_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=4, decode_responses=True)
            self.appl_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=0, decode_responses=True)
            self.state_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=6, decode_responses=True)
            logger.info("Connected to SONiC Redis databases")
        except Exception as e:
            logger.error(f"Failed to connect to Redis: {e}")
            sys.exit(1)
    
    def process_vlan_config(self, key, data):
        """Process VLAN configuration changes"""
        logger.info(f"Processing VLAN config: {key}")
        
        # Extract VLAN ID from key (VLAN|VlanXXX)
        vlan_name = key.split('|')[1]  # VlanXXX
        vlan_id = vlan_name.replace('Vlan', '')
        
        # Update APPL_DB with VLAN table entry
        appl_key = f"VLAN_TABLE:{vlan_name}"
        self.appl_db.hset(appl_key, "vlanid", vlan_id)
        
        # Update STATE_DB with VLAN state
        state_key = f"VLAN_TABLE|{vlan_name}"
        self.state_db.hset(state_key, "state", "ok")
        
        logger.info(f"VLAN {vlan_id} processed successfully")
    
    def process_vlan_member_config(self, key, data):
        """Process VLAN member configuration changes"""
        logger.info(f"Processing VLAN member config: {key}")
        
        # Extract VLAN and interface from key (VLAN_MEMBER|VlanXXX|InterfaceYYY)
        parts = key.split('|')
        vlan_name = parts[1]  # VlanXXX
        interface = parts[2]  # InterfaceYYY
        
        # Update APPL_DB with VLAN member entry
        appl_key = f"VLAN_MEMBER_TABLE:{vlan_name}:{interface}"
        tagging_mode = data.get('tagging_mode', 'untagged')
        self.appl_db.hset(appl_key, "tagging_mode", tagging_mode)
        
        logger.info(f"VLAN member {interface} added to {vlan_name} as {tagging_mode}")
    
    def process_port_config(self, key, data):
        """Process port configuration changes"""
        logger.info(f"Processing port config: {key}")
        
        # Extract port name from key (PORT|PortName)
        port_name = key.split('|')[1]
        
        # Update APPL_DB with port table entry
        appl_key = f"PORT_TABLE:{port_name}"
        for field, value in data.items():
            self.appl_db.hset(appl_key, field, value)
        
        # Update STATE_DB with port state
        state_key = f"PORT_TABLE|{port_name}"
        self.state_db.hset(state_key, "state", "ok")
        
        logger.info(f"Port {port_name} processed successfully")
    
    def monitor_config_changes(self):
        """Monitor CONFIG_DB for changes"""
        logger.info("Starting configuration monitoring...")
        
        # Subscribe to keyspace notifications
        pubsub = self.config_db.pubsub()
        pubsub.psubscribe('__keyspace@4__:*')
        
        for message in pubsub.listen():
            if not self.running:
                break
                
            if message['type'] == 'pmessage':
                key = message['channel'].replace('__keyspace@4__:', '')
                operation = message['data']
                
                logger.debug(f"Config change: {key} -> {operation}")
                
                # Process different types of configuration changes
                if key.startswith('VLAN|') and not key.startswith('VLAN_MEMBER|'):
                    if operation in ['hset', 'hmset']:
                        data = self.config_db.hgetall(key)
                        self.process_vlan_config(key, data)
                
                elif key.startswith('VLAN_MEMBER|'):
                    if operation in ['hset', 'hmset']:
                        data = self.config_db.hgetall(key)
                        self.process_vlan_member_config(key, data)
                
                elif key.startswith('PORT|'):
                    if operation in ['hset', 'hmset']:
                        data = self.config_db.hgetall(key)
                        self.process_port_config(key, data)
    
    def initialize_default_state(self):
        """Initialize default state in databases"""
        logger.info("Initializing default state...")
        
        # Initialize default ports in APPL_DB
        default_ports = ['Ethernet0', 'Ethernet4', 'Ethernet8', 'Ethernet12', 'Ethernet16', 'Ethernet20']
        
        for port in default_ports:
            appl_key = f"PORT_TABLE:{port}"
            self.appl_db.hset(appl_key, "admin_status", "up")
            self.appl_db.hset(appl_key, "oper_status", "up")
            self.appl_db.hset(appl_key, "speed", "25000")
            self.appl_db.hset(appl_key, "mtu", "9100")
            
            # State DB
            state_key = f"PORT_TABLE|{port}"
            self.state_db.hset(state_key, "state", "ok")
        
        logger.info("Default state initialized")
    
    def run(self):
        """Main orchestration agent loop"""
        logger.info("Mock SONiC Orchagent starting...")
        
        # Initialize default state
        self.initialize_default_state()
        
        try:
            # Monitor configuration changes
            self.monitor_config_changes()
        except KeyboardInterrupt:
            logger.info("Received interrupt signal")
        except Exception as e:
            logger.error(f"Orchagent error: {e}")
        finally:
            self.running = False
            logger.info("Mock SONiC Orchagent stopped")

def main():
    # Parse command line arguments
    log_file = None
    if len(sys.argv) > 2 and sys.argv[1] == '-d':
        log_file = sys.argv[2]
    
    # Configure file logging if specified
    if log_file:
        file_handler = logging.FileHandler(log_file)
        file_handler.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
        logger.addHandler(file_handler)
    
    # Create and run orchagent
    orchagent = MockOrchAgent()
    orchagent.run()

if __name__ == "__main__":
    main()
