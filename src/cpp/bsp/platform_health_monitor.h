/**
 * @file platform_health_monitor.h
 * @brief SONiC BSP Platform Health Monitor Header
 * @author SONiC POC Team
 * @date 2025-09-11
 */

#ifndef SONIC_BSP_PLATFORM_HEALTH_MONITOR_H
#define SONIC_BSP_PLATFORM_HEALTH_MONITOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace sonic {
namespace bsp {

/**
 * @brief System status enumeration
 */
enum class SystemStatus {
    HEALTHY,
    WARNING,
    CRITICAL,
    UNKNOWN
};

/**
 * @brief Alert type enumeration
 */
enum class AlertType {
    TEMPERATURE_HIGH,
    FAN_SPEED_LOW,
    POWER_HIGH,
    MEMORY_HIGH,
    SYSTEM_ERROR
};

/**
 * @brief Alert severity enumeration
 */
enum class AlertSeverity {
    INFO,
    WARNING,
    CRITICAL
};

/**
 * @brief Health thresholds structure
 */
struct HealthThresholds {
    float cpu_temp_max;        ///< Maximum CPU temperature (°C)
    uint32_t fan_speed_min;    ///< Minimum fan speed (RPM)
    float power_max;           ///< Maximum power consumption (W)
    float memory_usage_max;    ///< Maximum memory usage (%)
};

/**
 * @brief Health data structure
 */
struct HealthData {
    std::string timestamp;
    float cpu_temperature;                      ///< CPU temperature in Celsius
    std::map<std::string, uint32_t> fan_speeds; ///< Fan speeds in RPM
    float power_consumption;                    ///< Power consumption in Watts
    float memory_usage;                         ///< Memory usage percentage
    SystemStatus system_status;                 ///< Overall system status
    
    HealthData() : cpu_temperature(0.0f), power_consumption(0.0f), 
                   memory_usage(0.0f), system_status(SystemStatus::UNKNOWN) {}
};

/**
 * @brief Health alert structure
 */
struct HealthAlert {
    AlertType type;
    AlertSeverity severity;
    std::string message;
    std::string timestamp;
};

/**
 * @brief Platform Health Monitor class
 * 
 * This class provides comprehensive health monitoring for SONiC platform hardware.
 * It monitors temperature, fan speeds, power consumption, and memory usage.
 */
class PlatformHealthMonitor {
public:
    /**
     * @brief Constructor
     */
    PlatformHealthMonitor();
    
    /**
     * @brief Destructor
     */
    ~PlatformHealthMonitor();
    
    /**
     * @brief Start health monitoring
     * @return true if successful, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop health monitoring
     */
    void stop();
    
    /**
     * @brief Check if monitoring is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Get current health data
     * @return Current health data structure
     */
    HealthData getCurrentHealth() const;
    
    /**
     * @brief Get recent alerts
     * @param count Number of recent alerts to retrieve
     * @return Vector of recent alerts
     */
    std::vector<HealthAlert> getRecentAlerts(size_t count = 10) const;
    
    /**
     * @brief Set health thresholds
     * @param thresholds New threshold values
     */
    void setThresholds(const HealthThresholds& thresholds);
    
    /**
     * @brief Get current health thresholds
     * @return Current threshold values
     */
    HealthThresholds getThresholds() const;

private:
    /**
     * @brief Initialize platform hardware interfaces
     * @return true if successful, false otherwise
     */
    bool initializePlatform();
    
    /**
     * @brief Main monitoring loop
     */
    void monitoringLoop();
    
    /**
     * @brief Collect health data from hardware
     * @return Health data structure
     */
    HealthData collectHealthData();
    
    /**
     * @brief Read CPU temperature
     * @return Temperature in Celsius
     */
    float readCPUTemperature();
    
    /**
     * @brief Read fan speeds
     * @return Map of fan names to speeds (RPM)
     */
    std::map<std::string, uint32_t> readFanSpeeds();
    
    /**
     * @brief Read power consumption
     * @return Power consumption in Watts
     */
    float readPowerConsumption();
    
    /**
     * @brief Read memory usage
     * @return Memory usage percentage
     */
    float readMemoryUsage();
    
    /**
     * @brief Determine overall system status
     * @param health Health data to analyze
     * @return System status
     */
    SystemStatus determineSystemStatus(const HealthData& health);
    
    /**
     * @brief Check thresholds and generate alerts
     * @param health Health data to check
     */
    void checkThresholds(const HealthData& health);
    
    /**
     * @brief Log health data
     * @param health Health data to log
     */
    void logHealthData(const HealthData& health);

    /**
     * @brief Publish health data to Redis for Python API
     * @param health Health data to publish
     */
    void publishHealthData(const HealthData& health);
    
    /**
     * @brief Get current timestamp as string
     * @return Formatted timestamp string
     */
    std::string getCurrentTimestamp();
    
    /**
     * @brief Convert system status to string
     * @param status System status enum
     * @return String representation
     */
    std::string systemStatusToString(SystemStatus status);
    
    // Member variables
    std::atomic<bool> running_;
    bool platform_initialized_;
    std::unique_ptr<std::thread> monitoring_thread_;
    
    // Thread-safe data storage
    mutable std::mutex health_mutex_;
    HealthData current_health_;
    
    mutable std::mutex alerts_mutex_;
    std::vector<HealthAlert> alerts_;
    
    mutable std::mutex thresholds_mutex_;
    HealthThresholds thresholds_;
    
    // Disable copy constructor and assignment operator
    PlatformHealthMonitor(const PlatformHealthMonitor&) = delete;
    PlatformHealthMonitor& operator=(const PlatformHealthMonitor&) = delete;
};

} // namespace bsp
} // namespace sonic

#endif // SONIC_BSP_PLATFORM_HEALTH_MONITOR_H
