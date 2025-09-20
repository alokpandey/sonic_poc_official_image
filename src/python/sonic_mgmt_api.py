#!/usr/bin/env python3
"""
SONiC Management API
Provides management interface for SONiC POC
"""

import os
import sys
import json
import logging
from datetime import datetime
from flask import Flask, request, jsonify, render_template_string
from sonic_cli_wrapper import SONiCCLIWrapper
from sonic_redis_client import SONiCRedisClient

# Create log directory first
os.makedirs('/var/log/sonic-mgmt', exist_ok=True)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/var/log/sonic-mgmt/mgmt-api.log'),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

class SONiCManagementAPI:
    def __init__(self):
        self.sonic_host = os.getenv('SONIC_HOST', '172.20.0.10')
        self.sonic_ssh_port = int(os.getenv('SONIC_SSH_PORT', 2222))
        self.sonic_user = os.getenv('SONIC_USER', 'admin')
        self.sonic_pass = os.getenv('SONIC_PASS', 'YourPaSsWoRd')
        self.redis_host = os.getenv('REDIS_HOST', '172.21.0.11')
        self.redis_port = int(os.getenv('REDIS_PORT', 6379))
        
        # Initialize SONiC CLI wrapper
        self.cli = SONiCCLIWrapper(
            host=self.sonic_host,
            port=self.sonic_ssh_port,
            username=self.sonic_user,
            password=self.sonic_pass
        )
        
        # Initialize Redis client
        self.redis_client = SONiCRedisClient(
            host=self.redis_host,
            port=self.redis_port
        )
        
        logger.info("SONiC Management API initialized")

    def get_system_status(self):
        """Get overall system status"""
        try:
            status = {
                'sonic_connection': self.cli.test_connection(),
                'redis_connection': self.redis_client.test_connection(),
                'sonic_version': self.cli.get_sonic_version(),
                'system_info': self.redis_client.get_system_info(),
                'timestamp': datetime.utcnow().isoformat()
            }
            return status
        except Exception as e:
            logger.error(f"Error getting system status: {str(e)}")
            return {'error': str(e)}

# Initialize management API
mgmt_api = SONiCManagementAPI()

# Web interface template
WEB_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>SONiC Management Interface</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #2c3e50; color: white; padding: 20px; margin-bottom: 20px; }
        .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; }
        .status-ok { color: green; }
        .status-error { color: red; }
        .button { background: #3498db; color: white; padding: 10px 20px; border: none; cursor: pointer; }
        .button:hover { background: #2980b9; }
        pre { background: #f4f4f4; padding: 10px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>SONiC Network Operating System</h1>
        <p>Management Interface - POC Demo</p>
    </div>
    
    <div class="section">
        <h2>System Status</h2>
        <div id="system-status">Loading...</div>
        <button class="button" onclick="refreshStatus()">Refresh Status</button>
    </div>
    
    <div class="section">
        <h2>VLAN Management</h2>
        <button class="button" onclick="runVlanDemo()">Run VLAN Demo</button>
        <button class="button" onclick="showVlans()">Show VLANs</button>
        <div id="vlan-results"></div>
    </div>
    
    <div class="section">
        <h2>Port Management</h2>
        <button class="button" onclick="runPortDemo()">Run Port Demo</button>
        <button class="button" onclick="showPorts()">Show Ports</button>
        <div id="port-results"></div>
    </div>
    
    <div class="section">
        <h2>SONiC CLI</h2>
        <input type="text" id="cli-command" placeholder="Enter SONiC command" style="width: 300px;">
        <button class="button" onclick="executeCommand()">Execute</button>
        <pre id="cli-output"></pre>
    </div>

    <script>
        async function refreshStatus() {
            try {
                const response = await fetch('/api/v1/status');
                const data = await response.json();
                document.getElementById('system-status').innerHTML = 
                    '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
            } catch (error) {
                document.getElementById('system-status').innerHTML = 
                    '<span class="status-error">Error: ' + error.message + '</span>';
            }
        }
        
        async function runVlanDemo() {
            try {
                const response = await fetch('http://localhost:8091/api/v1/sai/demo/vlan-management', {
                    method: 'POST'
                });
                const data = await response.json();
                document.getElementById('vlan-results').innerHTML = 
                    '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
            } catch (error) {
                document.getElementById('vlan-results').innerHTML = 
                    '<span class="status-error">Error: ' + error.message + '</span>';
            }
        }
        
        async function showVlans() {
            try {
                const response = await fetch('http://localhost:8091/api/v1/sai/vlans');
                const data = await response.json();
                document.getElementById('vlan-results').innerHTML = 
                    '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
            } catch (error) {
                document.getElementById('vlan-results').innerHTML = 
                    '<span class="status-error">Error: ' + error.message + '</span>';
            }
        }
        
        async function runPortDemo() {
            try {
                const response = await fetch('http://localhost:8092/api/v1/bsp/demo/port-management', {
                    method: 'POST'
                });
                const data = await response.json();
                document.getElementById('port-results').innerHTML = 
                    '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
            } catch (error) {
                document.getElementById('port-results').innerHTML = 
                    '<span class="status-error">Error: ' + error.message + '</span>';
            }
        }
        
        async function showPorts() {
            try {
                const response = await fetch('http://localhost:8092/api/v1/bsp/ports');
                const data = await response.json();
                document.getElementById('port-results').innerHTML = 
                    '<pre>' + JSON.stringify(data, null, 2) + '</pre>';
            } catch (error) {
                document.getElementById('port-results').innerHTML = 
                    '<span class="status-error">Error: ' + error.message + '</span>';
            }
        }
        
        async function executeCommand() {
            const command = document.getElementById('cli-command').value;
            if (!command) return;
            
            try {
                const response = await fetch('/api/v1/cli/execute', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ command: command })
                });
                const data = await response.json();
                document.getElementById('cli-output').textContent = 
                    data.success ? data.output : 'Error: ' + data.error;
            } catch (error) {
                document.getElementById('cli-output').textContent = 'Error: ' + error.message;
            }
        }
        
        // Auto-refresh status on page load
        refreshStatus();
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    """Main management interface"""
    return render_template_string(WEB_TEMPLATE)

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    try:
        status = mgmt_api.get_system_status()
        return jsonify({
            'status': 'healthy' if status.get('sonic_connection') and status.get('redis_connection') else 'unhealthy',
            'details': status
        })
    except Exception as e:
        return jsonify({
            'status': 'unhealthy',
            'error': str(e)
        }), 500

@app.route('/api/v1/status', methods=['GET'])
def get_status():
    """Get system status"""
    try:
        status = mgmt_api.get_system_status()
        return jsonify(status)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/cli/execute', methods=['POST'])
def execute_cli_command():
    """Execute SONiC CLI command"""
    try:
        data = request.get_json()
        command = data.get('command')
        
        if not command:
            return jsonify({'error': 'Command is required'}), 400
        
        result = mgmt_api.cli.execute_command(command)
        return jsonify(result)
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    logger.info("Starting SONiC Management API")
    app.run(host='0.0.0.0', port=3000, debug=False)
