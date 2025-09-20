#!/usr/bin/env python3
"""
SONiC Redis Client
Provides Python interface to SONiC Redis databases using official SONiC schema
"""

import redis
import json
import logging
from typing import Dict, Any, List, Optional

logger = logging.getLogger(__name__)

class SONiCRedisClient:
    """Client for SONiC Redis databases"""
    
    # SONiC Database IDs
    CONFIG_DB = 4
    APPL_DB = 0
    STATE_DB = 6
    ASIC_DB = 1
    COUNTERS_DB = 2
    FLEX_COUNTER_DB = 5
    
    def __init__(self, host: str = 'localhost', port: int = 6379, password: Optional[str] = None,
                 use_docker: bool = True, container_name: str = 'sonic-vs-official'):
        self.host = host
        self.port = port
        self.password = password
        self.use_docker = use_docker
        self.container_name = container_name
        self.connections = {}
        
    def _execute_redis_command(self, command: str, db_id: int = 0) -> str:
        """Execute Redis command via Docker exec"""
        if self.use_docker:
            import subprocess
            try:
                cmd = ['docker', 'exec', self.container_name, 'redis-cli', '-n', str(db_id)] + command.split()
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    return result.stdout.strip()
                else:
                    logger.error(f"Redis command failed: {result.stderr}")
                    return ""
            except Exception as e:
                logger.error(f"Failed to execute Redis command: {e}")
                return ""
        else:
            # Fall back to direct Redis connection
            conn = self.get_connection(db_id)
            try:
                parts = command.split()
                if len(parts) == 1:
                    return str(conn.execute_command(parts[0]))
                else:
                    return str(conn.execute_command(*parts))
            except Exception as e:
                logger.error(f"Redis command failed: {e}")
                return ""

    def get_connection(self, db_id: int) -> redis.Redis:
        """Get Redis connection for specific database"""
        if self.use_docker:
            # For Docker mode, we don't use direct connections
            # Instead we use docker exec for Redis commands
            return None

        if db_id not in self.connections:
            try:
                self.connections[db_id] = redis.Redis(
                    host=self.host,
                    port=self.port,
                    db=db_id,
                    password=self.password,
                    decode_responses=True,
                    socket_timeout=10,
                    socket_connect_timeout=10
                )
                # Test connection
                self.connections[db_id].ping()
                logger.info(f"Connected to SONiC Redis DB {db_id}")
            except Exception as e:
                logger.error(f"Failed to connect to Redis DB {db_id}: {str(e)}")
                raise
        
        return self.connections[db_id]
    
    def test_connection(self) -> bool:
        """Test Redis connection"""
        try:
            if self.use_docker:
                result = self._execute_redis_command("ping", self.CONFIG_DB)
                return result.upper() == "PONG"
            else:
                conn = self.get_connection(self.CONFIG_DB)
                conn.ping()
                return True
        except:
            return False
    
    def get_config_db_entry(self, key: str) -> Optional[Dict[str, Any]]:
        """Get entry from CONFIG_DB"""
        try:
            conn = self.get_connection(self.CONFIG_DB)
            data = conn.hgetall(key)
            return dict(data) if data else None
        except Exception as e:
            logger.error(f"Error getting CONFIG_DB entry {key}: {str(e)}")
            return None
    
    def set_config_db_entry(self, key: str, data: Dict[str, Any]) -> bool:
        """Set entry in CONFIG_DB"""
        try:
            conn = self.get_connection(self.CONFIG_DB)
            conn.hmset(key, data)
            return True
        except Exception as e:
            logger.error(f"Error setting CONFIG_DB entry {key}: {str(e)}")
            return False
    
    def delete_config_db_entry(self, key: str) -> bool:
        """Delete entry from CONFIG_DB"""
        try:
            conn = self.get_connection(self.CONFIG_DB)
            conn.delete(key)
            return True
        except Exception as e:
            logger.error(f"Error deleting CONFIG_DB entry {key}: {str(e)}")
            return False
    
    def get_config_db_keys(self, pattern: str = "*") -> List[str]:
        """Get keys from CONFIG_DB matching pattern"""
        try:
            conn = self.get_connection(self.CONFIG_DB)
            return conn.keys(pattern)
        except Exception as e:
            logger.error(f"Error getting CONFIG_DB keys with pattern {pattern}: {str(e)}")
            return []
    
    def get_appl_db_entry(self, key: str) -> Optional[Dict[str, Any]]:
        """Get entry from APPL_DB"""
        try:
            conn = self.get_connection(self.APPL_DB)
            data = conn.hgetall(key)
            return dict(data) if data else None
        except Exception as e:
            logger.error(f"Error getting APPL_DB entry {key}: {str(e)}")
            return None
    
    def get_state_db_entry(self, key: str) -> Optional[Dict[str, Any]]:
        """Get entry from STATE_DB"""
        try:
            conn = self.get_connection(self.STATE_DB)
            data = conn.hgetall(key)
            return dict(data) if data else None
        except Exception as e:
            logger.error(f"Error getting STATE_DB entry {key}: {str(e)}")
            return None
    
    def get_counters_db_entry(self, key: str) -> Optional[Dict[str, Any]]:
        """Get entry from COUNTERS_DB"""
        try:
            conn = self.get_connection(self.COUNTERS_DB)
            data = conn.hgetall(key)
            return dict(data) if data else None
        except Exception as e:
            logger.error(f"Error getting COUNTERS_DB entry {key}: {str(e)}")
            return None
    
    def get_vlan_config(self, vlan_id: int) -> Optional[Dict[str, Any]]:
        """Get VLAN configuration from CONFIG_DB"""
        vlan_key = f"VLAN|Vlan{vlan_id}"
        return self.get_config_db_entry(vlan_key)
    
    def get_vlan_members(self, vlan_id: int) -> List[Dict[str, Any]]:
        """Get VLAN members from CONFIG_DB"""
        pattern = f"VLAN_MEMBER|Vlan{vlan_id}|*"
        member_keys = self.get_config_db_keys(pattern)
        
        members = []
        for key in member_keys:
            parts = key.split('|')
            if len(parts) >= 3:
                port_name = parts[2]
                member_data = self.get_config_db_entry(key)
                if member_data:
                    members.append({
                        'port': port_name,
                        'tagging_mode': member_data.get('tagging_mode', 'untagged')
                    })
        
        return members
    
    def get_interface_config(self, interface: str) -> Optional[Dict[str, Any]]:
        """Get interface configuration from CONFIG_DB"""
        interface_key = f"INTERFACE|{interface}"
        return self.get_config_db_entry(interface_key)
    
    def get_port_config(self, port: str) -> Optional[Dict[str, Any]]:
        """Get port configuration from CONFIG_DB"""
        port_key = f"PORT|{port}"
        return self.get_config_db_entry(port_key)
    
    def get_port_status(self, port: str) -> Optional[Dict[str, Any]]:
        """Get port status from APPL_DB"""
        port_key = f"PORT_TABLE:{port}"
        return self.get_appl_db_entry(port_key)
    
    def get_all_vlans(self) -> List[Dict[str, Any]]:
        """Get all VLANs from CONFIG_DB"""
        vlan_keys = self.get_config_db_keys("VLAN|*")
        vlans = []
        
        for key in vlan_keys:
            vlan_data = self.get_config_db_entry(key)
            if vlan_data:
                vlan_id = key.split('|')[1].replace('Vlan', '')
                vlans.append({
                    'vlan_id': int(vlan_id),
                    'config': vlan_data,
                    'members': self.get_vlan_members(int(vlan_id))
                })
        
        return vlans
    
    def get_all_ports(self) -> List[Dict[str, Any]]:
        """Get all ports from CONFIG_DB"""
        port_keys = self.get_config_db_keys("PORT|*")
        ports = []
        
        for key in port_keys:
            port_data = self.get_config_db_entry(key)
            if port_data:
                port_name = key.split('|')[1]
                # Get additional status from APPL_DB
                status_data = self.get_port_status(port_name)
                
                ports.append({
                    'port': port_name,
                    'config': port_data,
                    'status': status_data or {}
                })
        
        return ports
    
    def get_system_info(self) -> Dict[str, Any]:
        """Get system information from various databases"""
        try:
            info = {
                'config_db_keys': len(self.get_config_db_keys()),
                'vlans': len(self.get_config_db_keys("VLAN|*")),
                'ports': len(self.get_config_db_keys("PORT|*")),
                'interfaces': len(self.get_config_db_keys("INTERFACE|*")),
                'vlan_members': len(self.get_config_db_keys("VLAN_MEMBER|*"))
            }
            
            # Get device metadata
            device_metadata = self.get_config_db_entry("DEVICE_METADATA|localhost")
            if device_metadata:
                info['device_metadata'] = device_metadata
            
            return info
        except Exception as e:
            logger.error(f"Error getting system info: {str(e)}")
            return {}
    
    def subscribe_to_notifications(self, pattern: str, callback):
        """Subscribe to Redis keyspace notifications"""
        try:
            conn = self.get_connection(self.CONFIG_DB)
            pubsub = conn.pubsub()
            pubsub.psubscribe(pattern)
            
            for message in pubsub.listen():
                if message['type'] == 'pmessage':
                    callback(message)
        except Exception as e:
            logger.error(f"Error subscribing to notifications: {str(e)}")
    
    def close_connections(self):
        """Close all Redis connections"""
        for db_id, conn in self.connections.items():
            try:
                conn.close()
                logger.info(f"Closed connection to Redis DB {db_id}")
            except:
                pass
        self.connections.clear()
    
    def __del__(self):
        """Destructor"""
        self.close_connections()
