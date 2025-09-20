#!/usr/bin/env python3
"""
SONiC SAI Demo Application
Integrates with real SONiC components using official SONiC CLI and Redis
"""

import os
import sys
import json
import time
import logging
import subprocess
from datetime import datetime
from flask import Flask, request, jsonify
from sonic_cli_wrapper import SONiCCLIWrapper
from sonic_redis_client import SONiCRedisClient

# Create log directory first
os.makedirs('/var/log/sonic-sai-demo', exist_ok=True)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/var/log/sonic-sai-demo/sai-demo.log'),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

class SONiCSAIDemo:
    def __init__(self):
        self.sonic_host = os.getenv('SONIC_HOST', '172.25.0.10')
        self.sonic_ssh_port = int(os.getenv('SONIC_SSH_PORT', 22))
        self.sonic_user = os.getenv('SONIC_USER', 'admin')
        self.sonic_pass = os.getenv('SONIC_PASS', 'YourPaSsWoRd')
        self.redis_host = os.getenv('REDIS_HOST', '172.25.0.10')
        self.redis_port = int(os.getenv('REDIS_PORT', 6379))
        
        # Initialize SONiC CLI wrapper (using Docker exec)
        self.cli = SONiCCLIWrapper(
            host=self.sonic_host,
            port=self.sonic_ssh_port,
            username=self.sonic_user,
            password=self.sonic_pass,
            use_docker=True,
            container_name='sonic-vs-official'
        )

        # Initialize Redis client for SONiC database (using Docker exec)
        self.redis_client = SONiCRedisClient(
            host=self.redis_host,
            port=self.redis_port,
            use_docker=True,
            container_name='sonic-vs-official'
        )
        
        logger.info("SONiC SAI Demo initialized")

    def create_vlan_via_sonic(self, vlan_id, vlan_name):
        """Create VLAN using real SONiC CLI commands"""
        try:
            # Create VLAN using SONiC config command
            create_cmd = f"sudo config vlan add {vlan_id}"
            result = self.cli.execute_command(create_cmd)
            
            if result['success']:
                # Set VLAN description if supported
                desc_cmd = f"sudo config vlan description {vlan_id} '{vlan_name}'"
                self.cli.execute_command(desc_cmd)
                
                # Verify VLAN creation
                verify_cmd = "show vlan brief"
                verify_result = self.cli.execute_command(verify_cmd)
                
                if verify_result['success'] and str(vlan_id) in verify_result['output']:
                    logger.info(f"VLAN {vlan_id} ({vlan_name}) created successfully via SONiC CLI")
                    return {
                        'vlan_id': vlan_id,
                        'name': vlan_name,
                        'status': 'active',
                        'source': 'sonic_cli',
                        'created_at': datetime.utcnow().isoformat(),
                        'members': []
                    }
                else:
                    logger.error(f"VLAN {vlan_id} creation verification failed")
                    return None
            else:
                logger.error(f"Failed to create VLAN {vlan_id}: {result.get('error', 'Unknown error')}")
                return None
                
        except Exception as e:
            logger.error(f"Exception creating VLAN {vlan_id}: {str(e)}")
            return None

    def add_port_to_vlan(self, vlan_id, port_name, tagging='untagged'):
        """Add port to VLAN using SONiC CLI"""
        try:
            if tagging == 'tagged':
                cmd = f"sudo config vlan member add {vlan_id} {port_name} --tagged"
            else:
                cmd = f"sudo config vlan member add {vlan_id} {port_name}"
                
            result = self.cli.execute_command(cmd)
            
            if result['success']:
                logger.info(f"Port {port_name} added to VLAN {vlan_id} as {tagging}")
                return True
            else:
                logger.error(f"Failed to add port {port_name} to VLAN {vlan_id}")
                return False
                
        except Exception as e:
            logger.error(f"Exception adding port to VLAN: {str(e)}")
            return False

    def get_vlan_status_from_redis(self, vlan_id):
        """Get VLAN status from SONiC Redis database"""
        try:
            # Query VLAN table in CONFIG_DB
            vlan_key = f"VLAN|Vlan{vlan_id}"
            vlan_data = self.redis_client.get_config_db_entry(vlan_key)
            
            if vlan_data:
                # Query VLAN_MEMBER table for port members
                member_pattern = f"VLAN_MEMBER|Vlan{vlan_id}|*"
                members = self.redis_client.get_config_db_keys(member_pattern)
                
                member_list = []
                for member_key in members:
                    port_name = member_key.split('|')[-1]
                    member_data = self.redis_client.get_config_db_entry(member_key)
                    member_list.append({
                        'port': port_name,
                        'tagging_mode': member_data.get('tagging_mode', 'untagged')
                    })
                
                return {
                    'vlan_id': vlan_id,
                    'status': 'active',
                    'members': member_list,
                    'source': 'sonic_redis'
                }
            else:
                return None
                
        except Exception as e:
            logger.error(f"Exception querying VLAN status from Redis: {str(e)}")
            return None

    def get_interface_status(self):
        """Get interface status using SONiC CLI"""
        try:
            cmd = "show interfaces status"
            result = self.cli.execute_command(cmd)
            
            if result['success']:
                # Parse interface status output
                interfaces = []
                lines = result['output'].split('\n')
                
                for line in lines[2:]:  # Skip header lines
                    if line.strip():
                        parts = line.split()
                        if len(parts) >= 4:
                            interfaces.append({
                                'interface': parts[0],
                                'lanes': parts[1] if len(parts) > 1 else 'N/A',
                                'speed': parts[2] if len(parts) > 2 else 'N/A',
                                'mtu': parts[3] if len(parts) > 3 else 'N/A',
                                'admin_status': parts[4] if len(parts) > 4 else 'N/A',
                                'oper_status': parts[5] if len(parts) > 5 else 'N/A'
                            })
                
                return interfaces
            else:
                logger.error("Failed to get interface status")
                return []
                
        except Exception as e:
            logger.error(f"Exception getting interface status: {str(e)}")
            return []

# Initialize demo instance
demo = SONiCSAIDemo()

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    try:
        # Test SONiC connectivity
        sonic_status = demo.cli.test_connection()
        redis_status = demo.redis_client.test_connection()
        
        return jsonify({
            'status': 'healthy' if sonic_status and redis_status else 'unhealthy',
            'sonic_connection': sonic_status,
            'redis_connection': redis_status,
            'timestamp': datetime.utcnow().isoformat()
        })
    except Exception as e:
        return jsonify({
            'status': 'unhealthy',
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

@app.route('/api/v1/sai/demo/vlan-management', methods=['POST'])
def vlan_management_demo():
    """VLAN Management Demo using real SONiC CLI"""
    try:
        logger.info("Starting VLAN Management Demo with real SONiC")
        
        # Demo VLANs to create
        demo_vlans = [
            {'vlan_id': 100, 'name': 'Engineering'},
            {'vlan_id': 200, 'name': 'Sales'},
            {'vlan_id': 300, 'name': 'Management'}
        ]
        
        # Demo ports to configure
        demo_ports = ['Ethernet0', 'Ethernet4', 'Ethernet8', 'Ethernet12', 'Ethernet16', 'Ethernet20']
        
        created_vlans = []
        ports_configured = 0
        
        # Create VLANs using SONiC CLI
        for vlan_config in demo_vlans:
            vlan_result = demo.create_vlan_via_sonic(
                vlan_config['vlan_id'], 
                vlan_config['name']
            )
            
            if vlan_result:
                created_vlans.append(vlan_result)
                
                # Add ports to VLAN (first 2 ports per VLAN)
                vlan_ports = demo_ports[len(created_vlans)-1:len(created_vlans)+1]
                for port in vlan_ports:
                    if demo.add_port_to_vlan(vlan_config['vlan_id'], port):
                        ports_configured += 1
                        vlan_result['members'].append({
                            'port': port,
                            'tagging_mode': 'untagged'
                        })
            
            # Small delay between VLAN operations
            time.sleep(1)
        
        # Get final status from Redis
        for vlan in created_vlans:
            redis_status = demo.get_vlan_status_from_redis(vlan['vlan_id'])
            if redis_status:
                vlan.update(redis_status)
        
        response = {
            'demo': 'vlan-management',
            'status': 'completed',
            'timestamp': datetime.utcnow().isoformat(),
            'vlans_created': created_vlans,
            'ports_configured': ports_configured,
            'sonic_integration': {
                'cli_commands_used': True,
                'redis_database_queried': True,
                'real_sonic_components': True
            }
        }
        
        logger.info(f"VLAN Management Demo completed: {len(created_vlans)} VLANs created")
        return jsonify(response)
        
    except Exception as e:
        logger.error(f"VLAN Management Demo failed: {str(e)}")
        return jsonify({
            'demo': 'vlan-management',
            'status': 'failed',
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

@app.route('/api/v1/sai/interfaces', methods=['GET'])
def get_interfaces():
    """Get interface status from SONiC"""
    try:
        interfaces = demo.get_interface_status()
        return jsonify({
            'interfaces': interfaces,
            'count': len(interfaces),
            'source': 'sonic_cli',
            'timestamp': datetime.utcnow().isoformat()
        })
    except Exception as e:
        return jsonify({
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

@app.route('/api/v1/sai/vlans', methods=['GET'])
def get_vlans():
    """Get VLAN status from SONiC Redis"""
    try:
        # Get all VLANs from Redis CONFIG_DB
        vlan_keys = demo.redis_client.get_config_db_keys("VLAN|*")
        vlans = []
        
        for vlan_key in vlan_keys:
            vlan_id = vlan_key.split('|')[1].replace('Vlan', '')
            vlan_status = demo.get_vlan_status_from_redis(int(vlan_id))
            if vlan_status:
                vlans.append(vlan_status)
        
        return jsonify({
            'vlans': vlans,
            'count': len(vlans),
            'source': 'sonic_redis',
            'timestamp': datetime.utcnow().isoformat()
        })
    except Exception as e:
        return jsonify({
            'error': str(e),
            'timestamp': datetime.utcnow().isoformat()
        }), 500

if __name__ == '__main__':
    logger.info("Starting SONiC SAI Demo Application")
    app.run(host='0.0.0.0', port=8080, debug=False)
