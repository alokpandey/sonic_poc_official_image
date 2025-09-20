#!/bin/bash
# SONiC Health Check Script

set -e

SONIC_BIN="/opt/sonic/bin"
SONIC_LOG="/opt/sonic/var/log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[$(date '+%Y-%m-%d %H:%M:%S')] WARNING:${NC} $1"
}

error() {
    echo -e "${RED}[$(date '+%Y-%m-%d %H:%M:%S')] ERROR:${NC} $1"
}

# Check if SONiC main process is running
check_sonic_main() {
    if pgrep -f "sonic_poc" > /dev/null; then
        log "✅ SONiC main process is running"
        return 0
    else
        error "❌ SONiC main process is not running"
        return 1
    fi
}

# Check Redis connectivity
check_redis() {
    if redis-cli -h localhost -p 6379 ping > /dev/null 2>&1; then
        log "✅ Redis is responding"
        return 0
    else
        error "❌ Redis is not responding"
        return 1
    fi
}

# Check API endpoints
check_apis() {
    local api_status=0
    
    # Check BSP API
    if curl -s -f http://localhost:8080/health > /dev/null 2>&1; then
        log "✅ BSP API is responding"
    else
        error "❌ BSP API is not responding"
        api_status=1
    fi
    
    # Check SAI API
    if curl -s -f http://localhost:8081/health > /dev/null 2>&1; then
        log "✅ SAI API is responding"
    else
        error "❌ SAI API is not responding"
        api_status=1
    fi
    
    return $api_status
}

# Check log files
check_logs() {
    local log_status=0
    
    # Check if log files exist and are being written to
    local log_files=(
        "sonic-main.out.log"
        "bsp-api.out.log"
        "sai-api.out.log"
    )
    
    for log_file in "${log_files[@]}"; do
        local log_path="$SONIC_LOG/$log_file"
        if [[ -f "$log_path" ]]; then
            # Check if log file was modified in the last 5 minutes
            if [[ $(find "$log_path" -mmin -5) ]]; then
                log "✅ $log_file is active"
            else
                warn "⚠️  $log_file exists but may be stale"
            fi
        else
            error "❌ $log_file not found"
            log_status=1
        fi
    done
    
    return $log_status
}

# Check system resources
check_resources() {
    local resource_status=0
    
    # Check memory usage
    local mem_usage=$(free | grep Mem | awk '{printf "%.1f", $3/$2 * 100.0}')
    if (( $(echo "$mem_usage > 90" | bc -l) )); then
        error "❌ High memory usage: ${mem_usage}%"
        resource_status=1
    else
        log "✅ Memory usage: ${mem_usage}%"
    fi
    
    # Check disk usage
    local disk_usage=$(df /opt/sonic | tail -1 | awk '{print $5}' | sed 's/%//')
    if [[ $disk_usage -gt 90 ]]; then
        error "❌ High disk usage: ${disk_usage}%"
        resource_status=1
    else
        log "✅ Disk usage: ${disk_usage}%"
    fi
    
    return $resource_status
}

# Main health check function
main() {
    log "🔍 Starting SONiC health check..."
    
    local overall_status=0
    
    # Run all checks
    check_sonic_main || overall_status=1
    check_redis || overall_status=1
    check_apis || overall_status=1
    check_logs || overall_status=1
    check_resources || overall_status=1
    
    if [[ $overall_status -eq 0 ]]; then
        log "🎉 All health checks passed!"
        exit 0
    else
        error "💥 Some health checks failed!"
        exit 1
    fi
}

# Run main function
main "$@"
