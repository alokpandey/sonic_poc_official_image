#!/usr/bin/env python3
"""
SONiC Test Runner - Comprehensive testing of all use cases
"""

import os
import sys
import json
import time
import requests
import logging
import subprocess
from datetime import datetime
import asyncio
import aiohttp

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/opt/sonic/var/log/test-runner.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('TEST_RUNNER')

class SONiCTestRunner:
    """Comprehensive test runner for all SONiC use cases"""
    
    def __init__(self):
        self.bsp_api_url = "http://localhost:8080"
        self.sai_api_url = "http://localhost:8081"
        self.test_results = []
        
    def log_test_result(self, test_name, status, details=None, duration=None):
        """Log test result"""
        result = {
            'test': test_name,
            'status': status,
            'timestamp': datetime.now().isoformat(),
            'duration': duration,
            'details': details or {}
        }
        self.test_results.append(result)
        
        status_symbol = "✅" if status == "PASS" else "❌" if status == "FAIL" else "⚠️"
        logger.info(f"{status_symbol} {test_name}: {status}")
        
        if details:
            logger.info(f"   Details: {details}")
    
    def wait_for_services(self, timeout=60):
        """Wait for all services to be ready"""
        logger.info("Waiting for services to be ready...")
        
        services = [
            ("BSP API", f"{self.bsp_api_url}/health"),
            ("SAI API", f"{self.sai_api_url}/health")
        ]
        
        start_time = time.time()
        while time.time() - start_time < timeout:
            all_ready = True
            for service_name, url in services:
                try:
                    response = requests.get(url, timeout=5)
                    if response.status_code != 200:
                        all_ready = False
                        break
                except requests.RequestException:
                    all_ready = False
                    break
            
            if all_ready:
                logger.info("All services are ready!")
                return True
            
            time.sleep(2)
        
        logger.error("Services did not become ready within timeout")
        return False
    
    def test_bsp_health_monitoring(self):
        """Test BSP health monitoring functionality"""
        test_name = "BSP Health Monitoring"
        start_time = time.time()
        
        try:
            # Test health status endpoint
            response = requests.get(f"{self.bsp_api_url}/api/v1/bsp/health", timeout=10)
            if response.status_code != 200:
                raise Exception(f"Health endpoint returned {response.status_code}")
            
            health_data = response.json()
            required_fields = ['timestamp', 'cpu_temperature', 'fan_speeds', 'power_consumption', 'memory_usage']
            
            for field in required_fields:
                if field not in health_data:
                    raise Exception(f"Missing required field: {field}")
            
            # Test alerts endpoint
            response = requests.get(f"{self.bsp_api_url}/api/v1/bsp/alerts", timeout=10)
            if response.status_code != 200:
                raise Exception(f"Alerts endpoint returned {response.status_code}")
            
            # Run health monitoring demo
            response = requests.post(f"{self.bsp_api_url}/api/v1/bsp/demo/health-monitoring", timeout=30)
            if response.status_code != 200:
                raise Exception(f"Health monitoring demo failed: {response.status_code}")
            
            demo_result = response.json()
            
            duration = time.time() - start_time
            self.log_test_result(test_name, "PASS", {
                'health_fields_validated': len(required_fields),
                'demo_status': demo_result.get('status'),
                'health_checks': demo_result.get('results', {}).get('health_checks', 0)
            }, duration)
            
        except Exception as e:
            duration = time.time() - start_time
            self.log_test_result(test_name, "FAIL", {'error': str(e)}, duration)
    
    def test_bsp_led_control(self):
        """Test BSP LED control functionality"""
        test_name = "BSP LED Control"
        start_time = time.time()
        
        try:
            # Test LED control
            led_tests = [
                ('power', 'on', 'green'),
                ('status', 'blinking', 'amber'),
                ('alarm', 'off', None)
            ]
            
            for led_name, state, color in led_tests:
                # Set LED state
                payload = {'state': state}
                if color:
                    payload['color'] = color
                
                response = requests.post(
                    f"{self.bsp_api_url}/api/v1/bsp/led/{led_name}",
                    json=payload,
                    timeout=10
                )
                
                if response.status_code != 200:
                    raise Exception(f"LED control failed for {led_name}: {response.status_code}")
                
                # Verify LED state
                response = requests.get(f"{self.bsp_api_url}/api/v1/bsp/led/{led_name}", timeout=10)
                if response.status_code != 200:
                    raise Exception(f"LED status check failed for {led_name}")
                
                time.sleep(1)  # Brief delay between LED operations
            
            # Run LED control demo
            response = requests.post(f"{self.bsp_api_url}/api/v1/bsp/demo/led-control", timeout=30)
            if response.status_code != 200:
                raise Exception(f"LED control demo failed: {response.status_code}")
            
            demo_result = response.json()
            
            duration = time.time() - start_time
            self.log_test_result(test_name, "PASS", {
                'leds_tested': len(led_tests),
                'demo_status': demo_result.get('status'),
                'sequence_length': len(demo_result.get('sequence', []))
            }, duration)
            
        except Exception as e:
            duration = time.time() - start_time
            self.log_test_result(test_name, "FAIL", {'error': str(e)}, duration)
    
    def test_sai_vlan_management(self):
        """Test SAI VLAN management functionality"""
        test_name = "SAI VLAN Management"
        start_time = time.time()
        
        try:
            # Test VLAN creation
            test_vlans = [
                (100, 'Test_Engineering'),
                (200, 'Test_Sales'),
                (300, 'Test_Management')
            ]
            
            created_vlans = []
            for vlan_id, name in test_vlans:
                response = requests.post(
                    f"{self.sai_api_url}/api/v1/sai/vlans",
                    json={'vlan_id': vlan_id, 'name': name},
                    timeout=10
                )
                
                if response.status_code != 200:
                    raise Exception(f"VLAN creation failed for VLAN {vlan_id}: {response.status_code}")
                
                created_vlans.append(vlan_id)
            
            # Test port addition to VLANs
            port_additions = 0
            for vlan_id in created_vlans:
                # Add untagged port
                response = requests.post(
                    f"{self.sai_api_url}/api/v1/sai/vlans/{vlan_id}/members",
                    json={'port': f'Ethernet{vlan_id//100*4}', 'tagged': False},
                    timeout=10
                )
                
                if response.status_code == 200:
                    port_additions += 1
                
                # Add tagged port (trunk)
                response = requests.post(
                    f"{self.sai_api_url}/api/v1/sai/vlans/{vlan_id}/members",
                    json={'port': 'Ethernet20', 'tagged': True},
                    timeout=10
                )
                
                if response.status_code == 200:
                    port_additions += 1
            
            # Verify VLANs exist
            response = requests.get(f"{self.sai_api_url}/api/v1/sai/vlans", timeout=10)
            if response.status_code != 200:
                raise Exception(f"VLAN listing failed: {response.status_code}")
            
            vlans_data = response.json()
            vlans_found = len(vlans_data.get('vlans', {}))
            
            # Run VLAN management demo
            response = requests.post(f"{self.sai_api_url}/api/v1/sai/demo/vlan-management", timeout=30)
            if response.status_code != 200:
                raise Exception(f"VLAN management demo failed: {response.status_code}")
            
            demo_result = response.json()
            
            # Cleanup - delete test VLANs
            for vlan_id in created_vlans:
                requests.delete(f"{self.sai_api_url}/api/v1/sai/vlans/{vlan_id}", timeout=10)
            
            duration = time.time() - start_time
            self.log_test_result(test_name, "PASS", {
                'vlans_created': len(created_vlans),
                'ports_added': port_additions,
                'vlans_found': vlans_found,
                'demo_status': demo_result.get('status')
            }, duration)
            
        except Exception as e:
            duration = time.time() - start_time
            self.log_test_result(test_name, "FAIL", {'error': str(e)}, duration)
    
    def test_sai_l3_routing(self):
        """Test SAI L3 routing functionality"""
        test_name = "SAI L3 Routing"
        start_time = time.time()
        
        try:
            # Test route creation
            test_routes = [
                ('192.168.100.0/24', '10.0.1.1', 'Ethernet0'),
                ('192.168.200.0/24', '10.0.1.2', 'Ethernet4'),
                ('10.10.0.0/16', '10.0.1.5', 'Ethernet8')
            ]
            
            routes_added = 0
            for prefix, next_hop, interface in test_routes:
                response = requests.post(
                    f"{self.sai_api_url}/api/v1/sai/routes",
                    json={'prefix': prefix, 'next_hop': next_hop, 'interface': interface},
                    timeout=10
                )
                
                if response.status_code == 200:
                    routes_added += 1
            
            # Verify routes exist
            response = requests.get(f"{self.sai_api_url}/api/v1/sai/routes", timeout=10)
            if response.status_code != 200:
                raise Exception(f"Route listing failed: {response.status_code}")
            
            routes_data = response.json()
            routes_found = len(routes_data.get('routes', {}))
            
            # Run L3 routing demo
            response = requests.post(f"{self.sai_api_url}/api/v1/sai/demo/l3-routing", timeout=30)
            if response.status_code != 200:
                raise Exception(f"L3 routing demo failed: {response.status_code}")
            
            demo_result = response.json()
            
            duration = time.time() - start_time
            self.log_test_result(test_name, "PASS", {
                'routes_added': routes_added,
                'routes_found': routes_found,
                'demo_status': demo_result.get('status')
            }, duration)
            
        except Exception as e:
            duration = time.time() - start_time
            self.log_test_result(test_name, "FAIL", {'error': str(e)}, duration)
    
    def test_integration(self):
        """Test integration between BSP and SAI components"""
        test_name = "Integration Test"
        start_time = time.time()
        
        try:
            # Test that both APIs are responding
            bsp_response = requests.get(f"{self.bsp_api_url}/health", timeout=10)
            sai_response = requests.get(f"{self.sai_api_url}/health", timeout=10)
            
            if bsp_response.status_code != 200 or sai_response.status_code != 200:
                raise Exception("One or more APIs not responding")
            
            # Test concurrent operations
            # Create VLAN while monitoring health
            vlan_response = requests.post(
                f"{self.sai_api_url}/api/v1/sai/vlans",
                json={'vlan_id': 999, 'name': 'Integration_Test'},
                timeout=10
            )
            
            health_response = requests.get(f"{self.bsp_api_url}/api/v1/bsp/health", timeout=10)
            
            if vlan_response.status_code != 200 or health_response.status_code != 200:
                raise Exception("Concurrent operations failed")
            
            # Cleanup
            requests.delete(f"{self.sai_api_url}/api/v1/sai/vlans/999", timeout=10)
            
            duration = time.time() - start_time
            self.log_test_result(test_name, "PASS", {
                'apis_tested': 2,
                'concurrent_ops': 2
            }, duration)
            
        except Exception as e:
            duration = time.time() - start_time
            self.log_test_result(test_name, "FAIL", {'error': str(e)}, duration)
    
    def run_all_tests(self):
        """Run all test suites"""
        logger.info("🚀 Starting SONiC POC comprehensive test suite")
        logger.info("=" * 60)
        
        start_time = time.time()
        
        # Wait for services to be ready
        if not self.wait_for_services():
            logger.error("Services not ready, aborting tests")
            return False
        
        # Run all test suites
        test_suites = [
            self.test_bsp_health_monitoring,
            self.test_bsp_led_control,
            self.test_sai_vlan_management,
            self.test_sai_l3_routing,
            self.test_integration
        ]
        
        for test_suite in test_suites:
            try:
                test_suite()
            except Exception as e:
                logger.error(f"Test suite {test_suite.__name__} crashed: {e}")
            
            time.sleep(2)  # Brief pause between test suites
        
        # Generate test report
        total_duration = time.time() - start_time
        self.generate_test_report(total_duration)
        
        # Return overall success
        passed_tests = sum(1 for result in self.test_results if result['status'] == 'PASS')
        total_tests = len(self.test_results)
        
        logger.info("=" * 60)
        logger.info(f"🏁 Test suite completed: {passed_tests}/{total_tests} tests passed")
        
        return passed_tests == total_tests
    
    def generate_test_report(self, total_duration):
        """Generate comprehensive test report"""
        report = {
            'test_run': {
                'timestamp': datetime.now().isoformat(),
                'duration': total_duration,
                'total_tests': len(self.test_results),
                'passed': sum(1 for r in self.test_results if r['status'] == 'PASS'),
                'failed': sum(1 for r in self.test_results if r['status'] == 'FAIL'),
                'skipped': sum(1 for r in self.test_results if r['status'] == 'SKIP')
            },
            'test_results': self.test_results
        }
        
        # Save report to file
        report_file = f"/opt/sonic/var/log/test_report_{int(time.time())}.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        logger.info(f"📊 Test report saved to: {report_file}")
        
        # Print summary
        logger.info("\n📋 Test Summary:")
        for result in self.test_results:
            status_symbol = "✅" if result['status'] == "PASS" else "❌" if result['status'] == "FAIL" else "⚠️"
            duration_str = f" ({result['duration']:.2f}s)" if result['duration'] else ""
            logger.info(f"  {status_symbol} {result['test']}{duration_str}")

def main():
    """Main test runner entry point"""
    logger.info("SONiC POC Test Runner starting...")
    
    runner = SONiCTestRunner()
    success = runner.run_all_tests()
    
    exit_code = 0 if success else 1
    logger.info(f"Test runner exiting with code {exit_code}")
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
