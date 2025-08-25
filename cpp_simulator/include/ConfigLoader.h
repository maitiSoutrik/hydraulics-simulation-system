#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct SimulationConfig {
    // Simulation parameters
    double dt;
    double max_time;
    int udp_port;
    double telemetry_rate_hz;
    
    // System parameters
    double p1_initial;
    double p2_initial;
    double volume_cylinder;
    double beta;
    double P_max;
    double P_deadband;
    double safety_pressure_limit;
    
    // Valve parameters
    double fill_valve_area;
    double drain_valve_area;
    double relief_valve_area;
};

class ConfigLoader {
public:
    static SimulationConfig loadFromFile(const std::string& filename);
    static SimulationConfig loadFromJson(const nlohmann::json& config_json);
    
private:
    static void validateConfig(const SimulationConfig& config);
};
