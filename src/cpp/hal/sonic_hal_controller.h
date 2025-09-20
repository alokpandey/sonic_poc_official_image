#ifndef SONIC_HAL_CONTROLLER_H
#define SONIC_HAL_CONTROLLER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace sonic {
namespace hal {

// HAL Interface Status
enum class InterfaceStatus {
    UP,
    DOWN,
    UNKNOWN
};

// Fan Control Structure
struct FanInfo {
    int fan_id;
    int speed_rpm;
    int target_speed_rpm;
    bool is_present;
    std::string status;
};

// Temperature Sensor Structure
struct TempSensorInfo {
    int sensor_id;
    std::string name;
    float temperature;
    float high_threshold;
    float critical_threshold;
    std::string status;
};

// Power Supply Unit Structure
struct PSUInfo {
    int psu_id;
    std::string model;
    float voltage;
    float current;
    float power;
    bool is_present;
    std::string status;
};

// LED Control Structure
struct LEDInfo {
    std::string name;
    std::string color;
    std::string state;  // on, off, blinking
};

// Main HAL Controller Class
class SONiCHALController {
public:
    SONiCHALController();
    ~SONiCHALController();

    // Initialize HAL connection to SONiC
    bool initialize();
    void cleanup();

    // Interface Control
    bool setInterfaceStatus(const std::string& interface, InterfaceStatus status);
    InterfaceStatus getInterfaceStatus(const std::string& interface);
    bool setInterfaceSpeed(const std::string& interface, int speed_mbps);
    int getInterfaceSpeed(const std::string& interface);

    // Fan Control
    std::vector<FanInfo> getAllFans();
    bool setFanSpeed(int fan_id, int speed_percentage);
    FanInfo getFanInfo(int fan_id);
    bool setFanAutoMode(bool enable);

    // Temperature Monitoring
    std::vector<TempSensorInfo> getAllTempSensors();
    TempSensorInfo getTempSensorInfo(int sensor_id);
    float getCPUTemperature();
    float getBoardTemperature();

    // Power Management
    std::vector<PSUInfo> getAllPSUs();
    PSUInfo getPSUInfo(int psu_id);
    float getTotalPowerConsumption();

    // LED Control
    std::vector<LEDInfo> getAllLEDs();
    bool setLEDState(const std::string& led_name, const std::string& color, const std::string& state);
    LEDInfo getLEDInfo(const std::string& led_name);

    // System Information
    std::string getPlatformName();
    std::string getHardwareVersion();
    std::string getSerialNumber();

    // Test Functions
    bool runHALFunctionalTests();
    bool testFanControl();
    bool testTemperatureMonitoring();
    bool testPowerManagement();
    bool testLEDControl();
    bool testInterfaceControl();

private:
    bool m_initialized;
    std::string m_sonic_container_name;
    
    // Helper functions for SONiC communication
    bool executeSONiCCommand(const std::string& command, std::string& output);
    bool executeRedisCommand(const std::string& command, int db_id, std::string& output);
    bool setRedisValue(const std::string& key, const std::string& value, int db_id = 4);
    std::string getRedisValue(const std::string& key, int db_id = 4);
    
    // Platform-specific implementations
    bool initializePlatformHAL();
    bool detectPlatform();
    
    // Internal state
    std::string m_platform_name;
    std::map<std::string, InterfaceStatus> m_interface_status_cache;
    std::vector<FanInfo> m_fan_cache;
    std::vector<TempSensorInfo> m_temp_sensor_cache;
    std::vector<PSUInfo> m_psu_cache;
    std::vector<LEDInfo> m_led_cache;
};

} // namespace hal
} // namespace sonic

#endif // SONIC_HAL_CONTROLLER_H
