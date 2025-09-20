/**
 * @file sai_command_processor.cpp
 * @brief SAI Command Processor for Python-C++ Communication
 * @author SONiC POC Team
 * @date 2025-09-12
 */

#include "sai_vlan_manager.h"
#include "sai_adapter.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cstdio>

namespace sonic {
namespace sai {

class SAICommandProcessor {
private:
    SAIVLANManager vlan_manager_;
    bool running_;
    std::thread processor_thread_;

public:
    SAICommandProcessor() : running_(false) {}
    
    ~SAICommandProcessor() {
        stop();
    }
    
    bool start() {
        if (running_) return true;

        // Initialize SAI adapter first
        auto* sai_adapter = sonic::sai::SAIAdapter::getInstance();
        if (!sai_adapter || !sai_adapter->initialize()) {
            std::cerr << "Failed to initialize SAI adapter in command processor" << std::endl;
            return false;
        }
        std::cout << "SAI adapter initialized successfully in command processor" << std::endl;

        running_ = true;
        processor_thread_ = std::thread(&SAICommandProcessor::processCommands, this);
        std::cout << "SAI Command Processor started" << std::endl;
        return true;
    }
    
    void stop() {
        if (running_) {
            running_ = false;
            if (processor_thread_.joinable()) {
                processor_thread_.join();
            }
            std::cout << "SAI Command Processor stopped" << std::endl;
        }
    }
    
private:
    void processCommands() {
        while (running_) {
            try {
                // Check for commands from Python API via Redis
                std::string command = getNextCommand();
                if (!command.empty()) {
                    processCommand(command);
                }
                
                // Sleep briefly to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
            } catch (const std::exception& e) {
                std::cerr << "Error in command processor: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    std::string getNextCommand() {
        // Use redis-cli to get next command (in production, use hiredis library)
        std::string redis_cmd = "redis-cli -h localhost -p 6379 RPOP sonic:sai:commands";
        
        FILE* pipe = popen(redis_cmd.c_str(), "r");
        if (!pipe) return "";
        
        char buffer[1024];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        
        return result.empty() || result == "(nil)" ? "" : result;
    }
    
    void processCommand(const std::string& command_json) {
        std::cout << "Processing command: " << command_json << std::endl;
        
        // Parse JSON command (simple parsing for POC)
        // In production, use a proper JSON library like nlohmann/json
        
        if (command_json.find("create_vlan") != std::string::npos) {
            processCreateVLAN(command_json);
        } else if (command_json.find("delete_vlan") != std::string::npos) {
            processDeleteVLAN(command_json);
        } else {
            std::cerr << "Unknown command: " << command_json << std::endl;
        }
    }
    
    void processCreateVLAN(const std::string& command_json) {
        // Extract VLAN ID and name from JSON (simple parsing for POC)
        uint16_t vlan_id = 0;
        std::string vlan_name;

        std::cout << "Parsing JSON: " << command_json << std::endl;

        // Simple JSON parsing (in production, use proper JSON library)
        size_t vlan_id_pos = command_json.find("\"vlan_id\":");
        if (vlan_id_pos != std::string::npos) {
            size_t start = command_json.find_first_of("0123456789", vlan_id_pos);
            size_t end = command_json.find_first_not_of("0123456789", start);
            if (start != std::string::npos && end != std::string::npos) {
                vlan_id = std::stoi(command_json.substr(start, end - start));
            }
        }

        // Fixed VLAN name parsing
        size_t name_pos = command_json.find("\"name\": \"");
        if (name_pos == std::string::npos) {
            name_pos = command_json.find("\"name\":\"");
        }

        if (name_pos != std::string::npos) {
            size_t start = command_json.find("\"", name_pos + 7); // Find opening quote after "name":
            if (start != std::string::npos) {
                start++; // Move past the opening quote
                size_t end = command_json.find("\"", start); // Find closing quote
                if (end != std::string::npos) {
                    vlan_name = command_json.substr(start, end - start);
                }
            }
        }

        std::cout << "Extracted VLAN ID: " << vlan_id << ", Name: '" << vlan_name << "'" << std::endl;
        
        if (vlan_id > 0) {
            std::cout << "Creating VLAN " << vlan_id << " with name " << vlan_name << std::endl;

            bool success = vlan_manager_.createVLAN(vlan_id, vlan_name);

            std::cout << "VLAN creation result: " << (success ? "SUCCESS" : "FAILED") << std::endl;

            // Send response back to Python API
            std::ostringstream response;
            response << "{"
                    << "\"vlan_id\":" << vlan_id << ","
                    << "\"name\":\"" << vlan_name << "\","
                    << "\"status\":\"" << (success ? "active" : "error") << "\","
                    << "\"members\":[],"
                    << "\"created_at\":\"" << getCurrentTimestamp() << "\","
                    << "\"source\":\"cpp_component\""
                    << "}";

            std::cout << "Sending response: " << response.str() << std::endl;
            sendResponse("create_vlan", vlan_id, response.str());
        }
    }
    
    void processDeleteVLAN(const std::string& command_json) {
        // Similar implementation for delete VLAN
        std::cout << "Delete VLAN command received (not implemented in POC)" << std::endl;
    }
    
    void sendResponse(const std::string& action, uint16_t vlan_id, const std::string& response) {
        std::string response_key = "sonic:sai:response:" + action + ":" + std::to_string(vlan_id);
        std::string redis_cmd = "redis-cli -h localhost -p 6379 SETEX " + response_key + " 10 '" + response + "'";
        
        int result = system(redis_cmd.c_str());
        if (result == 0) {
            std::cout << "Sent response to Python API: " << response_key << std::endl;
        } else {
            std::cerr << "Failed to send response to Python API" << std::endl;
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return oss.str();
    }
};

} // namespace sai
} // namespace sonic

// Simple main function for testing
int main() {
    std::cout << "Starting SAI Command Processor..." << std::endl;
    
    sonic::sai::SAICommandProcessor processor;
    
    if (!processor.start()) {
        std::cerr << "Failed to start SAI Command Processor" << std::endl;
        return 1;
    }
    
    // Run for demonstration
    std::cout << "SAI Command Processor running. Press Ctrl+C to stop." << std::endl;
    
    try {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    processor.stop();
    return 0;
}
