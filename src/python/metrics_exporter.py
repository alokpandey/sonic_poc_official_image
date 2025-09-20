#!/usr/bin/env python3
"""
SONiC POC Metrics Exporter
Exports system and application metrics in Prometheus format
"""

import time
import json
import logging
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import psutil
import os
import sys

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class MetricsHandler(BaseHTTPRequestHandler):
    """HTTP handler for metrics endpoint"""
    
    def do_GET(self):
        """Handle GET requests"""
        try:
            if self.path == '/metrics':
                self.send_response(200)
                self.send_header('Content-type', 'text/plain; version=0.0.4; charset=utf-8')
                self.end_headers()
                
                metrics = self.generate_metrics()
                self.wfile.write(metrics.encode('utf-8'))
            elif self.path == '/health':
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                
                health = {"status": "healthy", "timestamp": time.time()}
                self.wfile.write(json.dumps(health).encode('utf-8'))
            else:
                self.send_response(404)
                self.end_headers()
                
        except Exception as e:
            logger.error(f"Error handling request: {e}")
            self.send_response(500)
            self.end_headers()
    
    def generate_metrics(self):
        """Generate Prometheus format metrics"""
        metrics = []
        
        # System metrics
        cpu_percent = psutil.cpu_percent(interval=1)
        memory = psutil.virtual_memory()
        disk = psutil.disk_usage('/')
        
        metrics.append(f"# HELP sonic_cpu_usage_percent CPU usage percentage")
        metrics.append(f"# TYPE sonic_cpu_usage_percent gauge")
        metrics.append(f"sonic_cpu_usage_percent {cpu_percent}")
        
        metrics.append(f"# HELP sonic_memory_usage_bytes Memory usage in bytes")
        metrics.append(f"# TYPE sonic_memory_usage_bytes gauge")
        metrics.append(f"sonic_memory_usage_bytes {memory.used}")
        
        metrics.append(f"# HELP sonic_memory_total_bytes Total memory in bytes")
        metrics.append(f"# TYPE sonic_memory_total_bytes gauge")
        metrics.append(f"sonic_memory_total_bytes {memory.total}")
        
        metrics.append(f"# HELP sonic_disk_usage_bytes Disk usage in bytes")
        metrics.append(f"# TYPE sonic_disk_usage_bytes gauge")
        metrics.append(f"sonic_disk_usage_bytes {disk.used}")
        
        metrics.append(f"# HELP sonic_disk_total_bytes Total disk space in bytes")
        metrics.append(f"# TYPE sonic_disk_total_bytes gauge")
        metrics.append(f"sonic_disk_total_bytes {disk.total}")
        
        # Application metrics
        metrics.append(f"# HELP sonic_uptime_seconds Application uptime in seconds")
        metrics.append(f"# TYPE sonic_uptime_seconds counter")
        metrics.append(f"sonic_uptime_seconds {time.time() - start_time}")
        
        # Network interface metrics (if available)
        try:
            net_io = psutil.net_io_counters()
            metrics.append(f"# HELP sonic_network_bytes_sent Network bytes sent")
            metrics.append(f"# TYPE sonic_network_bytes_sent counter")
            metrics.append(f"sonic_network_bytes_sent {net_io.bytes_sent}")
            
            metrics.append(f"# HELP sonic_network_bytes_recv Network bytes received")
            metrics.append(f"# TYPE sonic_network_bytes_recv counter")
            metrics.append(f"sonic_network_bytes_recv {net_io.bytes_recv}")
        except Exception as e:
            logger.warning(f"Could not get network metrics: {e}")
        
        # Process count
        try:
            process_count = len(psutil.pids())
            metrics.append(f"# HELP sonic_process_count Number of running processes")
            metrics.append(f"# TYPE sonic_process_count gauge")
            metrics.append(f"sonic_process_count {process_count}")
        except Exception as e:
            logger.warning(f"Could not get process count: {e}")
        
        return '\n'.join(metrics) + '\n'
    
    def log_message(self, format, *args):
        """Override to use our logger"""
        logger.info(f"{self.address_string()} - {format % args}")

class MetricsExporter:
    """Prometheus metrics exporter for SONiC POC"""
    
    def __init__(self, port=9090, host='0.0.0.0'):
        self.port = port
        self.host = host
        self.server = None
        self.running = False
        
    def start(self):
        """Start the metrics server"""
        try:
            self.server = HTTPServer((self.host, self.port), MetricsHandler)
            self.running = True
            
            logger.info(f"Starting metrics exporter on {self.host}:{self.port}")
            logger.info(f"Metrics available at http://{self.host}:{self.port}/metrics")
            logger.info(f"Health check available at http://{self.host}:{self.port}/health")
            
            # Start server in a separate thread
            server_thread = threading.Thread(target=self.server.serve_forever)
            server_thread.daemon = True
            server_thread.start()
            
            return True
            
        except Exception as e:
            logger.error(f"Failed to start metrics server: {e}")
            return False
    
    def stop(self):
        """Stop the metrics server"""
        if self.server:
            logger.info("Stopping metrics exporter")
            self.server.shutdown()
            self.server.server_close()
            self.running = False

# Global start time
start_time = time.time()

def main():
    """Main function"""
    logger.info("SONiC POC Metrics Exporter starting...")
    
    # Get configuration from environment
    port = int(os.getenv('METRICS_PORT', '9090'))
    host = os.getenv('METRICS_HOST', '0.0.0.0')
    
    # Create and start exporter
    exporter = MetricsExporter(port=port, host=host)
    
    if not exporter.start():
        logger.error("Failed to start metrics exporter")
        sys.exit(1)
    
    try:
        # Keep the main thread alive
        while exporter.running:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info("Received interrupt signal")
    finally:
        exporter.stop()
        logger.info("Metrics exporter stopped")

if __name__ == '__main__':
    main()
