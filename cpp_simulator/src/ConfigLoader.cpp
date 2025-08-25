#include "ConfigLoader.h"
#include <fstream>
#include <stdexcept>

SimulationConfig ConfigLoader::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }
    
    nlohmann::json config_json;
    file >> config_json;
    
    return loadFromJson(config_json);
}

SimulationConfig ConfigLoader::loadFromJson(const nlohmann::json& config_json) {
    SimulationConfig config;
    
    try {
        // Simulation parameters
        config.dt = config_json["simulation"]["dt"];
        config.max_time = config_json["simulation"]["max_time"];
        config.udp_port = config_json["simulation"]["udp_port"];
        config.telemetry_rate_hz = config_json["simulation"]["telemetry_rate_hz"];
        
        // System parameters
        config.p1_initial = config_json["system_parameters"]["p1_initial"];
        config.p2_initial = config_json["system_parameters"]["p2_initial"];
        config.volume_cylinder = config_json["system_parameters"]["volume_cylinder"];
        config.beta = config_json["system_parameters"]["beta"];
        config.P_max = config_json["system_parameters"]["P_max"];
        config.P_deadband = config_json["system_parameters"]["P_deadband"];
        config.safety_pressure_limit = config_json["system_parameters"]["safety_pressure_limit"];
        
        // Valve parameters
        config.fill_valve_area = config_json["valves"]["fill_valve"]["orifice_area"];
        config.drain_valve_area = config_json["valves"]["drain_valve"]["orifice_area"];
        config.relief_valve_area = config_json["valves"]["relief_valve"]["orifice_area"];
        
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
    
    validateConfig(config);
    return config;
}

void ConfigLoader::validateConfig(const SimulationConfig& config) {
    if (config.dt <= 0.0) {
        throw std::runtime_error("Invalid dt: must be positive");
    }
    if (config.max_time <= 0.0) {
        throw std::runtime_error("Invalid max_time: must be positive");
    }
    if (config.udp_port <= 0 || config.udp_port > 65535) {
        throw std::runtime_error("Invalid UDP port: must be between 1 and 65535");
    }
    if (config.telemetry_rate_hz <= 0.0) {
        throw std::runtime_error("Invalid telemetry_rate_hz: must be positive");
    }
    if (config.volume_cylinder <= 0.0) {
        throw std::runtime_error("Invalid volume_cylinder: must be positive");
    }
    if (config.beta <= 0.0) {
        throw std::runtime_error("Invalid beta: must be positive");
    }
    if (config.P_max <= 0.0) {
        throw std::runtime_error("Invalid P_max: must be positive");
    }
    if (config.P_deadband < 0.0) {
        throw std::runtime_error("Invalid P_deadband: must be non-negative");
    }
    if (config.fill_valve_area <= 0.0 || config.drain_valve_area <= 0.0 || config.relief_valve_area <= 0.0) {
        throw std::runtime_error("Invalid valve areas: must be positive");
    }
}
