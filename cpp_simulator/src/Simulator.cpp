#include "Simulator.h"
#include <stdexcept>
#include <cmath>

Simulator::Simulator(double dt, double max_time, double safety_pressure_limit)
    : dt_(dt), current_time_(0.0), max_time_(max_time), 
      safety_pressure_limit_(safety_pressure_limit) {
}

void Simulator::initialize(double p1_initial, double p2_initial, double volume_cylinder,
                          double beta, double p_max, double p_deadband,
                          double fill_area, double drain_area, double relief_area) {
    
    // Create system components
    // Accumulator has infinite volume (pressure source)
    accumulator_ = std::make_unique<Volume>(p1_initial, 1e6, beta);
    cylinder_ = std::make_unique<Volume>(p2_initial, volume_cylinder, beta);
    
    fill_valve_ = std::make_unique<FillValve>(fill_area);
    drain_valve_ = std::make_unique<FillValve>(drain_area);
    relief_valve_ = std::make_unique<ReliefValve>(relief_area, p_max, p_deadband);
    
    // Initialize state
    current_time_ = 0.0;
    updateState();
}

void Simulator::step() {
    if (isFinished()) {
        return;
    }
    
    double p1 = accumulator_->getPressure();
    double p2 = cylinder_->getPressure();
    
    // Update automatic valves
    relief_valve_->update(p2, 0.0, dt_);
    
    // Calculate flows
    double fill_flow = fill_valve_->calculateFlow(p1, p2);
    double drain_flow = drain_valve_->calculateFlow(p2, 0.0);  // Drain to tank (0 pressure)
    double relief_flow = relief_valve_->calculateFlow(p2, 0.0);  // Relief to tank
    
    // Net flow into cylinder
    double net_flow = fill_flow - drain_flow - relief_flow;
    
    // Update pressures
    cylinder_->updatePressure(net_flow, dt_);
    
    // Accumulator pressure remains constant (infinite volume assumption)
    
    // Update time and state
    current_time_ += dt_;
    updateState();
}

void Simulator::reset() {
    current_time_ = 0.0;
    if (accumulator_) accumulator_->reset(current_state_.p1);
    if (cylinder_) cylinder_->reset(0.0);
    if (fill_valve_) fill_valve_->close();
    if (drain_valve_) drain_valve_->close();
    if (relief_valve_) relief_valve_->close();
    updateState();
}

bool Simulator::setValveState(const std::string& valve_id, ValveState state) {
    if (valve_id == "fill_valve" && fill_valve_) {
        fill_valve_->setState(state);
        return true;
    } else if (valve_id == "drain_valve" && drain_valve_) {
        drain_valve_->setState(state);
        return true;
    } else if (valve_id == "relief_valve" && relief_valve_) {
        relief_valve_->setState(state);
        return true;
    }
    return false;
}

ValveState Simulator::getValveState(const std::string& valve_id) const {
    if (valve_id == "fill_valve" && fill_valve_) {
        return fill_valve_->getState();
    } else if (valve_id == "drain_valve" && drain_valve_) {
        return drain_valve_->getState();
    } else if (valve_id == "relief_valve" && relief_valve_) {
        return relief_valve_->getState();
    }
    return ValveState::CLOSED;
}

bool Simulator::checkSafetyViolation() const {
    return current_state_.p2 > safety_pressure_limit_;
}

bool Simulator::checkSimulationStability() const {
    // Check for NaN or infinite values
    return std::isfinite(current_state_.p1) && std::isfinite(current_state_.p2);
}

bool Simulator::checkPhysicalValidity() const {
    // Check for negative pressures
    return current_state_.p1 >= 0.0 && current_state_.p2 >= 0.0;
}

void Simulator::updateState() {
    current_state_.time = current_time_;
    current_state_.p1 = accumulator_ ? accumulator_->getPressure() : 0.0;
    current_state_.p2 = cylinder_ ? cylinder_->getPressure() : 0.0;
    
    current_state_.valve_states.clear();
    if (fill_valve_) current_state_.valve_states["fill_valve"] = fill_valve_->getState();
    if (drain_valve_) current_state_.valve_states["drain_valve"] = drain_valve_->getState();
    if (relief_valve_) current_state_.valve_states["relief_valve"] = relief_valve_->getState();
}

std::string Simulator::getValveStateString(ValveState state) const {
    return (state == ValveState::OPEN) ? "open" : "closed";
}
