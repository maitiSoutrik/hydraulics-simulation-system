#pragma once
#include "Volume.h"
#include "FillValve.h"
#include "ReliefValve.h"
#include <memory>
#include <string>
#include <map>

struct SimulationState {
    double time;
    double p1;  // Accumulator pressure
    double p2;  // Cylinder pressure
    std::map<std::string, ValveState> valve_states;
};

class Simulator {
private:
    // System components
    std::unique_ptr<Volume> accumulator_;
    std::unique_ptr<Volume> cylinder_;
    std::unique_ptr<FillValve> fill_valve_;
    std::unique_ptr<FillValve> drain_valve_;
    std::unique_ptr<ReliefValve> relief_valve_;
    
    // Simulation parameters
    double dt_;
    double current_time_;
    double max_time_;
    double safety_pressure_limit_;
    
    // State tracking
    SimulationState current_state_;
    
public:
    Simulator(double dt, double max_time, double safety_pressure_limit);
    
    // Initialization
    void initialize(double p1_initial, double p2_initial, double volume_cylinder, 
                   double beta, double p_max, double p_deadband,
                   double fill_area, double drain_area, double relief_area);
    
    // Simulation control
    void step();
    void reset();
    bool isFinished() const { return current_time_ >= max_time_; }
    
    // State access
    const SimulationState& getState() const { return current_state_; }
    double getCurrentTime() const { return current_time_; }
    
    // Valve control
    bool setValveState(const std::string& valve_id, ValveState state);
    ValveState getValveState(const std::string& valve_id) const;
    
    // Safety checks
    bool checkSafetyViolation() const;
    bool checkSimulationStability() const;
    bool checkPhysicalValidity() const;
    
private:
    void updateState();
    std::string getValveStateString(ValveState state) const;
};
