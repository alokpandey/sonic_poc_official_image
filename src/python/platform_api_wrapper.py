#!/usr/bin/env python3
"""
Platform API Wrapper
Provides abstraction for platform-specific hardware operations
"""

import os
import logging
import subprocess
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)

class PlatformAPIWrapper:
    """Wrapper for platform-specific hardware operations"""
    
    def __init__(self, use_docker: bool = False, container_name: str = 'sonic-vs-official'):
        self.use_docker = use_docker
        self.container_name = container_name
        self.platform_name = self._detect_platform()
        logger.info(f"Platform API initialized for: {self.platform_name} (Docker mode: {use_docker})")
    
    def _detect_platform(self) -> str:
        """Detect the current platform"""
        try:
            # Try to read platform from various sources
            platform_sources = [
                '/sys/devices/virtual/dmi/id/product_name',
                '/sys/devices/virtual/dmi/id/board_name',
                '/proc/device-tree/model'
            ]
            
            for source in platform_sources:
                if os.path.exists(source):
                    try:
                        with open(source, 'r') as f:
                            platform = f.read().strip()
                            if platform:
                                return platform
                    except:
                        continue
            
            # Fallback to generic detection
            return "x86_64-generic"
            
        except Exception as e:
            logger.warning(f"Could not detect platform: {e}")
            return "unknown"
    
    def get_platform_name(self) -> str:
        """Get platform name"""
        return self.platform_name
    
    def get_chassis_info(self) -> Dict[str, Any]:
        """Get chassis information"""
        try:
            info = {
                'name': self.platform_name,
                'model': self._get_system_info('product_name'),
                'serial': self._get_system_info('product_serial'),
                'part_number': self._get_system_info('product_version'),
                'base_mac': self._get_base_mac(),
                'system_eeprom': self._get_system_eeprom()
            }
            return info
        except Exception as e:
            logger.error(f"Error getting chassis info: {e}")
            return {}
    
    def _get_system_info(self, info_type: str) -> Optional[str]:
        """Get system information from DMI"""
        try:
            dmi_path = f'/sys/devices/virtual/dmi/id/{info_type}'
            if os.path.exists(dmi_path):
                with open(dmi_path, 'r') as f:
                    return f.read().strip()
        except:
            pass
        return None
    
    def _get_base_mac(self) -> Optional[str]:
        """Get base MAC address"""
        try:
            # Try to get MAC from network interfaces
            result = subprocess.run(['cat', '/sys/class/net/eth0/address'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        return "00:00:00:00:00:00"
    
    def _get_system_eeprom(self) -> Dict[str, Any]:
        """Get system EEPROM data"""
        # Simulate EEPROM data for demo
        return {
            'tlv_header': {
                'signature': 'TlvInfo',
                'version': 1,
                'total_length': 256
            },
            'tlvs': [
                {'type': 0x21, 'name': 'Product Name', 'value': self.platform_name},
                {'type': 0x22, 'name': 'Part Number', 'value': 'PN-12345'},
                {'type': 0x23, 'name': 'Serial Number', 'value': 'SN-67890'},
                {'type': 0x24, 'name': 'Base MAC Address', 'value': self._get_base_mac()},
                {'type': 0x25, 'name': 'Manufacture Date', 'value': '01/01/2023'},
                {'type': 0x26, 'name': 'Device Version', 'value': '1.0'},
                {'type': 0x27, 'name': 'Label Revision', 'value': 'A'},
                {'type': 0x28, 'name': 'Platform Name', 'value': self.platform_name},
                {'type': 0x29, 'name': 'ONIE Version', 'value': '2023.05'},
                {'type': 0x2A, 'name': 'MAC Addresses', 'value': '256'},
                {'type': 0x2B, 'name': 'Manufacturer', 'value': 'SONiC Demo'},
                {'type': 0x2C, 'name': 'Country Code', 'value': 'US'}
            ]
        }
    
    def get_thermal_info(self) -> List[Dict[str, Any]]:
        """Get thermal sensor information"""
        try:
            thermals = []
            
            # Try to read from hwmon
            hwmon_path = '/sys/class/hwmon'
            if os.path.exists(hwmon_path):
                for hwmon_dir in os.listdir(hwmon_path):
                    hwmon_full_path = os.path.join(hwmon_path, hwmon_dir)
                    if os.path.isdir(hwmon_full_path):
                        thermal_data = self._read_hwmon_thermal(hwmon_full_path)
                        if thermal_data:
                            thermals.extend(thermal_data)
            
            # If no real sensors found, provide simulated data
            if not thermals:
                thermals = [
                    {'name': 'CPU Core', 'temperature': 45.2, 'high_threshold': 85.0, 'critical_threshold': 95.0},
                    {'name': 'Ambient', 'temperature': 28.5, 'high_threshold': 50.0, 'critical_threshold': 60.0},
                    {'name': 'ASIC', 'temperature': 52.1, 'high_threshold': 90.0, 'critical_threshold': 100.0}
                ]
            
            return thermals
            
        except Exception as e:
            logger.error(f"Error getting thermal info: {e}")
            return []
    
    def _read_hwmon_thermal(self, hwmon_path: str) -> List[Dict[str, Any]]:
        """Read thermal data from hwmon"""
        thermals = []
        try:
            # Look for temperature input files
            for file in os.listdir(hwmon_path):
                if file.startswith('temp') and file.endswith('_input'):
                    temp_file = os.path.join(hwmon_path, file)
                    try:
                        with open(temp_file, 'r') as f:
                            temp_millidegrees = int(f.read().strip())
                            temp_celsius = temp_millidegrees / 1000.0
                            
                            # Try to get sensor name
                            name_file = temp_file.replace('_input', '_label')
                            sensor_name = f"Sensor {file}"
                            if os.path.exists(name_file):
                                with open(name_file, 'r') as nf:
                                    sensor_name = nf.read().strip()
                            
                            thermals.append({
                                'name': sensor_name,
                                'temperature': temp_celsius,
                                'high_threshold': 85.0,
                                'critical_threshold': 95.0
                            })
                    except:
                        continue
        except:
            pass
        return thermals
    
    def get_fan_info(self) -> List[Dict[str, Any]]:
        """Get fan information"""
        try:
            fans = []
            
            # Try to read from hwmon
            hwmon_path = '/sys/class/hwmon'
            if os.path.exists(hwmon_path):
                for hwmon_dir in os.listdir(hwmon_path):
                    hwmon_full_path = os.path.join(hwmon_path, hwmon_dir)
                    if os.path.isdir(hwmon_full_path):
                        fan_data = self._read_hwmon_fans(hwmon_full_path)
                        if fan_data:
                            fans.extend(fan_data)
            
            # If no real fans found, provide simulated data
            if not fans:
                fans = [
                    {'name': 'Fan1', 'speed': 3200, 'target_speed': 3200, 'status': 'ok'},
                    {'name': 'Fan2', 'speed': 3150, 'target_speed': 3200, 'status': 'ok'},
                    {'name': 'Fan3', 'speed': 3180, 'target_speed': 3200, 'status': 'ok'},
                    {'name': 'Fan4', 'speed': 3220, 'target_speed': 3200, 'status': 'ok'}
                ]
            
            return fans
            
        except Exception as e:
            logger.error(f"Error getting fan info: {e}")
            return []
    
    def _read_hwmon_fans(self, hwmon_path: str) -> List[Dict[str, Any]]:
        """Read fan data from hwmon"""
        fans = []
        try:
            # Look for fan input files
            for file in os.listdir(hwmon_path):
                if file.startswith('fan') and file.endswith('_input'):
                    fan_file = os.path.join(hwmon_path, file)
                    try:
                        with open(fan_file, 'r') as f:
                            speed = int(f.read().strip())
                            
                            # Try to get fan name
                            name_file = fan_file.replace('_input', '_label')
                            fan_name = f"Fan {file}"
                            if os.path.exists(name_file):
                                with open(name_file, 'r') as nf:
                                    fan_name = nf.read().strip()
                            
                            fans.append({
                                'name': fan_name,
                                'speed': speed,
                                'target_speed': speed,
                                'status': 'ok' if speed > 0 else 'failed'
                            })
                    except:
                        continue
        except:
            pass
        return fans
    
    def get_psu_info(self) -> List[Dict[str, Any]]:
        """Get PSU information"""
        try:
            # Simulate PSU data for demo
            psus = [
                {
                    'name': 'PSU1',
                    'model': 'PWR-500W-AC',
                    'serial': 'PSU1234567',
                    'status': 'ok',
                    'presence': True,
                    'voltage': 12.1,
                    'current': 8.5,
                    'power': 102.85,
                    'temperature': 42.3
                },
                {
                    'name': 'PSU2', 
                    'model': 'PWR-500W-AC',
                    'serial': 'PSU2345678',
                    'status': 'ok',
                    'presence': True,
                    'voltage': 12.0,
                    'current': 8.3,
                    'power': 99.6,
                    'temperature': 41.8
                }
            ]
            return psus
            
        except Exception as e:
            logger.error(f"Error getting PSU info: {e}")
            return []
    
    def set_fan_speed(self, fan_name: str, speed_percent: int) -> bool:
        """Set fan speed"""
        try:
            logger.info(f"Setting {fan_name} speed to {speed_percent}%")
            # Simulate fan speed setting
            return True
        except Exception as e:
            logger.error(f"Error setting fan speed: {e}")
            return False
    
    def get_led_status(self, led_name: str) -> Optional[str]:
        """Get LED status"""
        try:
            # Simulate LED status
            led_states = ['off', 'green', 'red', 'amber', 'blue']
            return 'green'  # Default to green for demo
        except Exception as e:
            logger.error(f"Error getting LED status: {e}")
            return None
    
    def set_led_status(self, led_name: str, status: str) -> bool:
        """Set LED status"""
        try:
            logger.info(f"Setting {led_name} LED to {status}")
            # Simulate LED control
            return True
        except Exception as e:
            logger.error(f"Error setting LED status: {e}")
            return False
