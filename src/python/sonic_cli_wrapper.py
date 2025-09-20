#!/usr/bin/env python3
"""
SONiC CLI Wrapper
Provides Python interface to SONiC CLI commands via SSH
"""

import paramiko
import logging
import time
from typing import Dict, Any, Optional

logger = logging.getLogger(__name__)

class SONiCCLIWrapper:
    def __init__(self, host: str, port: int = 22, username: str = 'admin', password: str = 'YourPaSsWoRd',
                 use_docker: bool = True, container_name: str = 'sonic-vs-official'):
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        self.use_docker = use_docker
        self.container_name = container_name
        self.ssh_client = None
        self.connected = False
        
    def connect(self) -> bool:
        """Establish connection to SONiC device (SSH or Docker exec)"""
        if self.use_docker:
            # For Docker-based SONiC, we'll use docker exec instead of SSH
            try:
                import subprocess
                result = subprocess.run(['docker', 'exec', self.container_name, 'echo', 'test'],
                                      capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    self.connected = True
                    logger.info(f"Connected to SONiC container: {self.container_name}")
                    return True
                else:
                    logger.error(f"Docker exec failed: {result.stderr}")
                    self.connected = False
                    return False
            except Exception as e:
                logger.error(f"Failed to connect to SONiC container: {e}")
                self.connected = False
                return False
        else:
            # Traditional SSH connection
            try:
                self.ssh_client = paramiko.SSHClient()
                self.ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

                # Connect with timeout
                self.ssh_client.connect(
                    hostname=self.host,
                    port=self.port,
                    username=self.username,
                    password=self.password,
                    timeout=30,
                    allow_agent=False,
                    look_for_keys=False
                )

                self.connected = True
                logger.info(f"Connected to SONiC device at {self.host}:{self.port}")
                return True

            except Exception as e:
                logger.error(f"Failed to connect to SONiC device: {str(e)}")
                self.connected = False
                return False
    
    def disconnect(self):
        """Close SSH connection"""
        if self.ssh_client:
            self.ssh_client.close()
            self.connected = False
            logger.info("Disconnected from SONiC device")
    
    def execute_command(self, command: str, timeout: int = 30) -> Dict[str, Any]:
        """Execute command on SONiC device"""
        if not self.connected:
            if not self.connect():
                return {
                    'success': False,
                    'error': 'Failed to connect to SONiC device',
                    'output': '',
                    'command': command
                }
        
        try:
            logger.info(f"Executing SONiC command: {command}")

            if self.use_docker:
                # Execute command using docker exec
                import subprocess
                result = subprocess.run(['docker', 'exec', self.container_name, 'bash', '-c', command],
                                      capture_output=True, text=True, timeout=timeout)

                output = result.stdout.strip()
                error = result.stderr.strip()
                exit_code = result.returncode
                success = (exit_code == 0)
            else:
                # Execute command using SSH
                stdin, stdout, stderr = self.ssh_client.exec_command(command, timeout=timeout)

                # Get output
                output = stdout.read().decode('utf-8').strip()
                error = stderr.read().decode('utf-8').strip()
                exit_code = stdout.channel.recv_exit_status()
                success = (exit_code == 0)

            result = {
                'success': success,
                'exit_code': exit_code,
                'output': output,
                'error': error,
                'command': command
            }

            if success:
                logger.info(f"Command executed successfully: {command}")
            else:
                logger.error(f"Command failed: {command}, Error: {error}")

            return result
            
        except Exception as e:
            logger.error(f"Exception executing command '{command}': {str(e)}")
            return {
                'success': False,
                'error': str(e),
                'output': '',
                'command': command
            }
    
    def test_connection(self) -> bool:
        """Test connection to SONiC device"""
        try:
            result = self.execute_command('echo "connection_test"')
            return result['success'] and 'connection_test' in result['output']
        except:
            return False
    
    def get_sonic_version(self) -> Optional[str]:
        """Get SONiC version"""
        try:
            result = self.execute_command('show version')
            if result['success']:
                # Parse version from output
                lines = result['output'].split('\n')
                for line in lines:
                    if 'SONiC Software Version' in line:
                        return line.split(':')[-1].strip()
            return None
        except:
            return None
    
    def show_vlan_brief(self) -> Dict[str, Any]:
        """Execute 'show vlan brief' command"""
        return self.execute_command('show vlan brief')
    
    def show_interfaces_status(self) -> Dict[str, Any]:
        """Execute 'show interfaces status' command"""
        return self.execute_command('show interfaces status')
    
    def show_ip_interfaces(self) -> Dict[str, Any]:
        """Execute 'show ip interfaces' command"""
        return self.execute_command('show ip interfaces')
    
    def show_mac_address_table(self) -> Dict[str, Any]:
        """Execute 'show mac' command"""
        return self.execute_command('show mac')
    
    def config_vlan_add(self, vlan_id: int) -> Dict[str, Any]:
        """Add VLAN using config command"""
        return self.execute_command(f'sudo config vlan add {vlan_id}')
    
    def config_vlan_del(self, vlan_id: int) -> Dict[str, Any]:
        """Delete VLAN using config command"""
        return self.execute_command(f'sudo config vlan del {vlan_id}')
    
    def config_vlan_member_add(self, vlan_id: int, interface: str, tagged: bool = False) -> Dict[str, Any]:
        """Add interface to VLAN"""
        tag_option = '--tagged' if tagged else ''
        return self.execute_command(f'sudo config vlan member add {vlan_id} {interface} {tag_option}')
    
    def config_vlan_member_del(self, vlan_id: int, interface: str) -> Dict[str, Any]:
        """Remove interface from VLAN"""
        return self.execute_command(f'sudo config vlan member del {vlan_id} {interface}')
    
    def config_interface_startup(self, interface: str) -> Dict[str, Any]:
        """Bring interface up"""
        return self.execute_command(f'sudo config interface startup {interface}')
    
    def config_interface_shutdown(self, interface: str) -> Dict[str, Any]:
        """Bring interface down"""
        return self.execute_command(f'sudo config interface shutdown {interface}')
    
    def config_interface_speed(self, interface: str, speed: str) -> Dict[str, Any]:
        """Set interface speed"""
        return self.execute_command(f'sudo config interface speed {interface} {speed}')
    
    def config_save(self) -> Dict[str, Any]:
        """Save configuration"""
        return self.execute_command('sudo config save -y')
    
    def config_reload(self) -> Dict[str, Any]:
        """Reload configuration"""
        return self.execute_command('sudo config reload -y')
    
    def show_system_status(self) -> Dict[str, Any]:
        """Show system status"""
        return self.execute_command('show system-health summary')
    
    def show_processes(self) -> Dict[str, Any]:
        """Show running processes"""
        return self.execute_command('show processes summary')
    
    def __enter__(self):
        """Context manager entry"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.disconnect()
    
    def __del__(self):
        """Destructor"""
        if hasattr(self, 'connected') and self.connected:
            self.disconnect()
