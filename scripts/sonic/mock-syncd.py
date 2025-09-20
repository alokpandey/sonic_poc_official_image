#!/usr/bin/env python3
"""
Mock SONiC Syncd (Sync Daemon)
Simulates the SONiC sync daemon that interfaces with SAI
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
logger = logging.getLogger('syncd')

class MockSyncd:
    def __init__(self):
        self.redis_host = 'localhost'
        self.redis_port = 6379
        self.running = True
        self.sai_objects = {}  # Track SAI objects
        self.next_oid = 0x1000000000000001
        
        # Connect to SONiC databases
        try:
            self.appl_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=0, decode_responses=True)
            self.asic_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=1, decode_responses=True)
            self.counters_db = redis.Redis(host=self.redis_host, port=self.redis_port, db=2, decode_responses=True)
            logger.info("Connected to SONiC Redis databases")
        except Exception as e:
            logger.error(f"Failed to connect to Redis: {e}")
            sys.exit(1)
    
    def generate_oid(self):
        """Generate unique SAI object ID"""
        oid = self.next_oid
        self.next_oid += 1
        return f"oid:0x{oid:x}"
    
    def create_sai_vlan(self, vlan_id):
        """Create SAI VLAN object"""
        oid = self.generate_oid()
        
        # Store in ASIC_DB
        asic_key = f"ASIC_STATE:SAI_OBJECT_TYPE_VLAN:{oid}"
        self.asic_db.hset(asic_key, "SAI_VLAN_ATTR_VLAN_ID", vlan_id)
        
        # Track object
        self.sai_objects[f"vlan_{vlan_id}"] = oid
        
        logger.info(f"Created SAI VLAN {vlan_id} with OID {oid}")
        return oid
    
    def create_sai_vlan_member(self, vlan_id, port_name, tagging_mode):
        """Create SAI VLAN member object"""
        oid = self.generate_oid()
        
        # Get VLAN OID
        vlan_oid = self.sai_objects.get(f"vlan_{vlan_id}")
        if not vlan_oid:
            vlan_oid = self.create_sai_vlan(vlan_id)
        
        # Get port OID (create if needed)
        port_oid = self.sai_objects.get(f"port_{port_name}")
        if not port_oid:
            port_oid = self.create_sai_port(port_name)
        
        # Store in ASIC_DB
        asic_key = f"ASIC_STATE:SAI_OBJECT_TYPE_VLAN_MEMBER:{oid}"
        self.asic_db.hset(asic_key, "SAI_VLAN_MEMBER_ATTR_VLAN_ID", vlan_oid)
        self.asic_db.hset(asic_key, "SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID", port_oid)
        
        tagging_value = "SAI_VLAN_TAGGING_MODE_TAGGED" if tagging_mode == "tagged" else "SAI_VLAN_TAGGING_MODE_UNTAGGED"
        self.asic_db.hset(asic_key, "SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE", tagging_value)
        
        # Track object
        self.sai_objects[f"vlan_member_{vlan_id}_{port_name}"] = oid
        
        logger.info(f"Created SAI VLAN member for VLAN {vlan_id}, port {port_name} with OID {oid}")
        return oid
    
    def create_sai_port(self, port_name):
        """Create SAI port object"""
        oid = self.generate_oid()
        
        # Store in ASIC_DB
        asic_key = f"ASIC_STATE:SAI_OBJECT_TYPE_PORT:{oid}"
        self.asic_db.hset(asic_key, "SAI_PORT_ATTR_ADMIN_STATE", "true")
        self.asic_db.hset(asic_key, "SAI_PORT_ATTR_SPEED", "25000")
        self.asic_db.hset(asic_key, "SAI_PORT_ATTR_MTU", "9100")
        
        # Track object
        self.sai_objects[f"port_{port_name}"] = oid
        
        logger.info(f"Created SAI port {port_name} with OID {oid}")
        return oid
    
    def process_vlan_table_entry(self, key, data):
        """Process VLAN table entry from APPL_DB"""
        logger.info(f"Processing VLAN table entry: {key}")
        
        # Extract VLAN name from key (VLAN_TABLE:VlanXXX)
        vlan_name = key.split(':')[1]  # VlanXXX
        vlan_id = vlan_name.replace('Vlan', '')
        
        # Create SAI VLAN object
        self.create_sai_vlan(vlan_id)
        
        # Update counters
        counter_key = f"COUNTERS:oid:0x{self.sai_objects[f'vlan_{vlan_id}'].split(':')[1]}"
        self.counters_db.hset(counter_key, "SAI_VLAN_STAT_IN_OCTETS", "0")
        self.counters_db.hset(counter_key, "SAI_VLAN_STAT_OUT_OCTETS", "0")
        self.counters_db.hset(counter_key, "SAI_VLAN_STAT_IN_PACKETS", "0")
        self.counters_db.hset(counter_key, "SAI_VLAN_STAT_OUT_PACKETS", "0")
    
    def process_vlan_member_table_entry(self, key, data):
        """Process VLAN member table entry from APPL_DB"""
        logger.info(f"Processing VLAN member table entry: {key}")
        
        # Extract VLAN and port from key (VLAN_MEMBER_TABLE:VlanXXX:PortYYY)
        parts = key.split(':')
        vlan_name = parts[1]  # VlanXXX
        port_name = parts[2]  # PortYYY
        
        vlan_id = vlan_name.replace('Vlan', '')
        tagging_mode = data.get('tagging_mode', 'untagged')
        
        # Create SAI VLAN member object
        self.create_sai_vlan_member(vlan_id, port_name, tagging_mode)
    
    def process_port_table_entry(self, key, data):
        """Process port table entry from APPL_DB"""
        logger.info(f"Processing port table entry: {key}")
        
        # Extract port name from key (PORT_TABLE:PortName)
        port_name = key.split(':')[1]
        
        # Create or update SAI port object
        port_oid = self.sai_objects.get(f"port_{port_name}")
        if not port_oid:
            port_oid = self.create_sai_port(port_name)
        
        # Update port attributes in ASIC_DB
        asic_key = f"ASIC_STATE:SAI_OBJECT_TYPE_PORT:{port_oid}"
        
        if 'admin_status' in data:
            admin_state = "true" if data['admin_status'] == 'up' else "false"
            self.asic_db.hset(asic_key, "SAI_PORT_ATTR_ADMIN_STATE", admin_state)
        
        if 'speed' in data:
            self.asic_db.hset(asic_key, "SAI_PORT_ATTR_SPEED", data['speed'])
        
        if 'mtu' in data:
            self.asic_db.hset(asic_key, "SAI_PORT_ATTR_MTU", data['mtu'])
        
        # Update counters
        counter_key = f"COUNTERS:{port_oid}"
        self.counters_db.hset(counter_key, "SAI_PORT_STAT_IF_IN_OCTETS", "0")
        self.counters_db.hset(counter_key, "SAI_PORT_STAT_IF_OUT_OCTETS", "0")
        self.counters_db.hset(counter_key, "SAI_PORT_STAT_IF_IN_UCAST_PKTS", "0")
        self.counters_db.hset(counter_key, "SAI_PORT_STAT_IF_OUT_UCAST_PKTS", "0")
    
    def monitor_appl_db_changes(self):
        """Monitor APPL_DB for changes"""
        logger.info("Starting APPL_DB monitoring...")
        
        # Subscribe to keyspace notifications
        pubsub = self.appl_db.pubsub()
        pubsub.psubscribe('__keyspace@0__:*')
        
        for message in pubsub.listen():
            if not self.running:
                break
                
            if message['type'] == 'pmessage':
                key = message['channel'].replace('__keyspace@0__:', '')
                operation = message['data']
                
                logger.debug(f"APPL_DB change: {key} -> {operation}")
                
                # Process different types of application changes
                if key.startswith('VLAN_TABLE:') and operation in ['hset', 'hmset']:
                    data = self.appl_db.hgetall(key)
                    self.process_vlan_table_entry(key, data)
                
                elif key.startswith('VLAN_MEMBER_TABLE:') and operation in ['hset', 'hmset']:
                    data = self.appl_db.hgetall(key)
                    self.process_vlan_member_table_entry(key, data)
                
                elif key.startswith('PORT_TABLE:') and operation in ['hset', 'hmset']:
                    data = self.appl_db.hgetall(key)
                    self.process_port_table_entry(key, data)
    
    def initialize_default_sai_objects(self):
        """Initialize default SAI objects"""
        logger.info("Initializing default SAI objects...")
        
        # Create default switch object
        switch_oid = self.generate_oid()
        switch_key = f"ASIC_STATE:SAI_OBJECT_TYPE_SWITCH:{switch_oid}"
        self.asic_db.hset(switch_key, "SAI_SWITCH_ATTR_INIT_SWITCH", "true")
        self.sai_objects["switch"] = switch_oid
        
        # Create default ports
        default_ports = ['Ethernet0', 'Ethernet4', 'Ethernet8', 'Ethernet12', 'Ethernet16', 'Ethernet20']
        for port in default_ports:
            self.create_sai_port(port)
        
        logger.info("Default SAI objects initialized")
    
    def run(self):
        """Main sync daemon loop"""
        logger.info("Mock SONiC Syncd starting...")
        
        # Initialize default SAI objects
        self.initialize_default_sai_objects()
        
        try:
            # Monitor APPL_DB changes
            self.monitor_appl_db_changes()
        except KeyboardInterrupt:
            logger.info("Received interrupt signal")
        except Exception as e:
            logger.error(f"Syncd error: {e}")
        finally:
            self.running = False
            logger.info("Mock SONiC Syncd stopped")

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
    
    # Create and run syncd
    syncd = MockSyncd()
    syncd.run()

if __name__ == "__main__":
    main()
