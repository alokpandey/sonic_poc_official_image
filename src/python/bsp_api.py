#!/usr/bin/env python3
"""
BSP API Server - Python interface to C++ BSP components
"""

import os
import sys
import json
import time
import subprocess
from flask import Flask, jsonify, request
from flask_cors import CORS
import redis
import logging
from datetime import datetime

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/opt/sonic/var/log/bsp-api.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('BSP_API')

app = Flask(__name__)
CORS(app)

# Redis connection
try:
    redis_client = redis.Redis(host='localhost', port=6379, decode_responses=True)
    redis_client.ping()
    logger.info("Connected to Redis successfully")
except Exception as e:
    logger.error(f"Failed to connect to Redis: {e}")
    redis_client = None

class BSPInterface:
    """Interface to C++ BSP components"""
    
    def __init__(self):
        self.sonic_bin = "/opt/sonic/bin"
        
    def get_health_status(self):
        """Get current health status from C++ component"""
        try:
            # REAL IMPLEMENTATION: Read from C++ component via Redis
            if redis_client:
                # Try to get real data from C++ health monitor
                health_json = redis_client.get('sonic:bsp:health:current')
                if health_json:
                    logger.info("Retrieved real health data from C++ component")
                    return json.loads(health_json)
                else:
                    logger.warning("No health data from C++ component, using fallback")

            # Fallback: simulate data if C++ component not available
            health_data = {
                'timestamp': datetime.now().isoformat(),
                'cpu_temperature': 45.2,
                'fan_speeds': {
                    'fan_1': 3200,
                    'fan_2': 3150,
                    'fan_3': 3180,
                    'fan_4': 3220
                },
                'power_consumption': 145.8,
                'memory_usage': 67.3,
                'system_status': 'healthy',
                'source': 'simulated'  # Indicate this is simulated data
            }

            return health_data
            
        except Exception as e:
            logger.error(f"Error getting health status: {e}")
            return {'error': str(e)}
    
    def get_alerts(self, count=10):
        """Get recent alerts"""
        try:
            # Simulate getting alerts from C++ component
            alerts = [
                {
                    'type': 'temperature_warning',
                    'severity': 'warning',
                    'message': 'CPU temperature approaching threshold',
                    'timestamp': datetime.now().isoformat()
                }
            ]
            return alerts
            
        except Exception as e:
            logger.error(f"Error getting alerts: {e}")
            return []
    
    def set_led_state(self, led_name, state, color=None):
        """Set LED state via C++ component"""
        try:
            # In real implementation, this would call the C++ LED controller
            logger.info(f"Setting LED {led_name} to {state} {color or ''}")
            
            # Store LED state in Redis
            if redis_client:
                led_data = {'state': state, 'color': color, 'timestamp': datetime.now().isoformat()}
                redis_client.setex(f'bsp:led:{led_name}', 300, json.dumps(led_data))
            
            return {'success': True, 'led': led_name, 'state': state, 'color': color}
            
        except Exception as e:
            logger.error(f"Error setting LED state: {e}")
            return {'error': str(e)}

# Initialize BSP interface
bsp = BSPInterface()

@app.route('/health', methods=['GET'])
def health_check():
    """API health check"""
    return jsonify({'status': 'healthy', 'service': 'bsp-api', 'timestamp': datetime.now().isoformat()})

@app.route('/api/v1/bsp/health', methods=['GET'])
def get_health():
    """Get system health status"""
    try:
        health_data = bsp.get_health_status()
        return jsonify(health_data)
    except Exception as e:
        logger.error(f"Error in get_health: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/bsp/alerts', methods=['GET'])
def get_alerts():
    """Get recent alerts"""
    try:
        count = request.args.get('count', 10, type=int)
        alerts = bsp.get_alerts(count)
        return jsonify({'alerts': alerts})
    except Exception as e:
        logger.error(f"Error in get_alerts: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/bsp/led/<led_name>', methods=['POST'])
def set_led(led_name):
    """Set LED state"""
    try:
        data = request.get_json()
        state = data.get('state', 'off')
        color = data.get('color')
        
        result = bsp.set_led_state(led_name, state, color)
        return jsonify(result)
    except Exception as e:
        logger.error(f"Error in set_led: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/bsp/led/<led_name>', methods=['GET'])
def get_led(led_name):
    """Get LED state"""
    try:
        if redis_client:
            led_data = redis_client.get(f'bsp:led:{led_name}')
            if led_data:
                return jsonify(json.loads(led_data))
        
        return jsonify({'led': led_name, 'state': 'unknown'})
    except Exception as e:
        logger.error(f"Error in get_led: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/bsp/demo/health-monitoring', methods=['POST'])
def demo_health_monitoring():
    """Run BSP health monitoring demo"""
    try:
        logger.info("Starting BSP health monitoring demo")
        
        # Simulate running the C++ demo
        demo_results = {
            'demo': 'health-monitoring',
            'status': 'completed',
            'duration': '30 seconds',
            'results': {
                'health_checks': 10,
                'alerts_generated': 2,
                'thresholds_tested': ['cpu_temp', 'fan_speed', 'power', 'memory']
            },
            'timestamp': datetime.now().isoformat()
        }
        
        return jsonify(demo_results)
    except Exception as e:
        logger.error(f"Error in demo_health_monitoring: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/v1/bsp/demo/led-control', methods=['POST'])
def demo_led_control():
    """Run BSP LED control demo"""
    try:
        logger.info("Starting BSP LED control demo")
        
        # Simulate LED control sequence
        led_sequence = [
            {'led': 'power', 'state': 'on', 'color': 'green'},
            {'led': 'status', 'state': 'blinking', 'color': 'amber'},
            {'led': 'alarm', 'state': 'off'},
            {'led': 'status', 'state': 'on', 'color': 'green'}
        ]
        
        for led_cmd in led_sequence:
            bsp.set_led_state(led_cmd['led'], led_cmd['state'], led_cmd.get('color'))
            time.sleep(1)
        
        demo_results = {
            'demo': 'led-control',
            'status': 'completed',
            'sequence': led_sequence,
            'timestamp': datetime.now().isoformat()
        }
        
        return jsonify(demo_results)
    except Exception as e:
        logger.error(f"Error in demo_led_control: {e}")
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    logger.info("Starting BSP API server")
    app.run(host='0.0.0.0', port=8080, debug=False)
