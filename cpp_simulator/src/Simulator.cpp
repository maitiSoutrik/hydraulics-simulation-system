#include "Simulator.h"
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

Simulator::Simulator(double dt, double max_time, double safety_pressure_limit)
    : dt_(dt), current_time_(0.0), max_time_(max_time), 
      safety_pressure_limit_(safety_pressure_limit) {
      
    // Validate constructor parameters
    if (dt <= 0.0) {
        throw HydraulicException(
            "Time step must be positive, got: " + std::to_string(dt), 
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (max_time < 0.0) {
        throw HydraulicException(
            "Maximum time must be non-negative, got: " + std::to_string(max_time), 
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (safety_pressure_limit <= 0.0) {
        throw HydraulicException(
            "Safety pressure limit must be positive, got: " + std::to_string(safety_pressure_limit), 
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    // Set default operational limits
    limits_.max_pressure = safety_pressure_limit_ * 1.1;  // 10% above safety limit
    limits_.min_pressure = 0.0;  // No negative pressures
    limits_.max_pressure_rate = 1.0e6;  // 1 MPa/s
    limits_.max_flow_rate = 0.01;  // 10 L/s
    limits_.min_bulk_modulus = 1.0e6;  // 1 MPa
    limits_.max_time_step = 0.01;  // 10 ms
    
    // Validate time step against limits
    if (dt > limits_.max_time_step) {
        recordWarning(
            "Time step (" + std::to_string(dt) + " s) exceeds recommended maximum (" + 
            std::to_string(limits_.max_time_step) + " s)",
            "Simulator"
        );
    }
}

void Simulator::setOperationalLimits(const SimulationLimits& limits) {
    // Validate limits
    if (limits.max_pressure <= 0.0) {
        throw HydraulicException(
            "Maximum pressure must be positive",
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (limits.max_pressure < limits.min_pressure) {
        throw HydraulicException(
            "Maximum pressure must be greater than minimum pressure",
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (limits.max_flow_rate <= 0.0) {
        throw HydraulicException(
            "Maximum flow rate must be positive",
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (limits.min_bulk_modulus <= 0.0) {
        throw HydraulicException(
            "Minimum bulk modulus must be positive",
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    if (limits.max_time_step <= 0.0) {
        throw HydraulicException(
            "Maximum time step must be positive",
            HydraulicException::ErrorCategory::PARAMETER_ERROR,
            "Simulator"
        );
    }
    
    // Apply validated limits
    limits_ = limits;
}

void Simulator::initialize(double p1_initial, double p2_initial, double volume_cylinder,
                          double beta, double p_max, double p_deadband,
                          double fill_area, double drain_area, double relief_area) {
    
    // Input parameter validation
    std::ostringstream errorMsg;
    bool hasError = false;
    
    if (p1_initial < 0.0) {
        errorMsg << "Initial accumulator pressure cannot be negative (got: " << p1_initial << " Pa). ";
        hasError = true;
    }
    
    if (p2_initial < 0.0) {
        errorMsg << "Initial cylinder pressure cannot be negative (got: " << p2_initial << " Pa). ";
        hasError = true;
    }
    
    if (volume_cylinder <= 0.0) {
        errorMsg << "Cylinder volume must be positive (got: " << volume_cylinder << " m³). ";
        hasError = true;
    }
    
    if (beta <= 0.0) {
        errorMsg << "Bulk modulus must be positive (got: " << beta << " Pa). ";
        hasError = true;
    } else if (beta < limits_.min_bulk_modulus) {
        recordWarning(
            "Bulk modulus (" + std::to_string(beta) + " Pa) is below recommended minimum (" + 
            std::to_string(limits_.min_bulk_modulus) + " Pa)", 
            "Simulator"
        );
    }
    
    if (p_max <= 0.0) {
        errorMsg << "Relief valve max pressure must be positive (got: " << p_max << " Pa). ";
        hasError = true;
    }
    
    if (p_deadband < 0.0) {
        errorMsg << "Relief valve deadband cannot be negative (got: " << p_deadband << " Pa). ";
        hasError = true;
    }
    
    if (fill_area < 0.0) {
        errorMsg << "Fill valve area cannot be negative (got: " << fill_area << " m²). ";
        hasError = true;
    }
    
    if (drain_area < 0.0) {
        errorMsg << "Drain valve area cannot be negative (got: " << drain_area << " m²). ";
        hasError = true;
    }
    
    if (relief_area < 0.0) {
        errorMsg << "Relief valve area cannot be negative (got: " << relief_area << " m²). ";
        hasError = true;
    }
    
    if (hasError) {
        throw HydraulicException(
            errorMsg.str(),
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator"
        );
    }
    
    try {
        // Create system components
        // Accumulator has large volume (pressure source)
        accumulator_ = std::make_unique<Volume>(p1_initial, 1e6, beta);
        cylinder_ = std::make_unique<Volume>(p2_initial, volume_cylinder, beta);
        
        fill_valve_ = std::make_unique<FillValve>(fill_area);
        drain_valve_ = std::make_unique<FillValve>(drain_area);
        relief_valve_ = std::make_unique<ReliefValve>(relief_area, p_max, p_deadband);
        
        if (!accumulator_ || !cylinder_ || !fill_valve_ || !drain_valve_ || !relief_valve_) {
            throw HydraulicException(
                "Failed to allocate memory for simulation components",
                HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
                "Simulator"
            );
        }
        
        // Initialize state
        current_time_ = 0.0;
        initialized_ = true;
        updateState();
    } catch (const std::bad_alloc&) {
        throw HydraulicException(
            "Memory allocation failure during initialization",
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator"
        );
    } catch (const std::exception& e) {
        throw HydraulicException(
            std::string("Initialization error: ") + e.what(),
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator"
        );
    }
}

void Simulator::step() {
    // Check initialization status
    if (!initialized_) {
        throw HydraulicException(
            "Simulator not initialized. Call initialize() before step()",
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator",
            current_time_
        );
    }
    
    // Check if simulation is already complete
    if (isFinished()) {
        return;
    }
    
    // Check for component initialization
    checkComponentsInitialized();
    
    try {
        double p1 = accumulator_->getPressure();
        double p2 = cylinder_->getPressure();
        
        // Validate current state
        validateParameters(p1, p2);
        
        // Update automatic valves
        relief_valve_->update(p2, 0.0, dt_);
        
        // Calculate flows
        double fill_flow = fill_valve_->calculateFlow(p1, p2);
        double drain_flow = drain_valve_->calculateFlow(p2, 0.0);  // Drain to tank (0 pressure)
        double relief_flow = relief_valve_->calculateFlow(p2, 0.0);  // Relief to tank
        
        // Check flow limits for stability
        if (std::abs(fill_flow) > limits_.max_flow_rate ||
            std::abs(drain_flow) > limits_.max_flow_rate ||
            std::abs(relief_flow) > limits_.max_flow_rate) {
            recordWarning(
                "Flow rate exceeds limits - simulation may become unstable",
                "Simulator"
            );
        }
        
        // Net flow into cylinder
        double net_flow = fill_flow - drain_flow - relief_flow;
        
        // Save previous pressure for rate-of-change check
        double prev_p2 = p2;
        
        // Update pressures
        cylinder_->updatePressure(net_flow, dt_);
        
        // Check pressure rate of change
        double new_p2 = cylinder_->getPressure();
        double pressure_rate = std::abs((new_p2 - prev_p2) / dt_);
        if (pressure_rate > limits_.max_pressure_rate) {
            recordWarning(
                "Pressure changing too rapidly (" + std::to_string(pressure_rate) + 
                " Pa/s), may indicate instability",
                "Cylinder"
            );
        }
        
        // Update time and state
        current_time_ += dt_;
        updateState();
        
        // Check safety violations after update
        if (checkSafetyViolation()) {
            current_state_.error = "Safety pressure limit exceeded";
            current_state_.is_valid = false;
            throw HydraulicException(
                "Safety pressure limit exceeded: " + std::to_string(current_state_.p2) + 
                " Pa > " + std::to_string(safety_pressure_limit_) + " Pa",
                HydraulicException::ErrorCategory::PRESSURE_VIOLATION,
                "Cylinder",
                current_time_
            );
        }
        
        // Check numerical stability
        if (!checkSimulationStability()) {
            current_state_.error = "Simulation numerically unstable (NaN or infinite values detected)";
            current_state_.is_valid = false;
            throw HydraulicException(
                "Simulation numerically unstable (NaN or infinite values detected)",
                HydraulicException::ErrorCategory::SIMULATION_INSTABILITY,
                "Simulator",
                current_time_
            );
        }
        
        // Check physical validity
        if (!checkPhysicalValidity()) {
            current_state_.error = "Simulation violates physical constraints (negative pressure)";
            current_state_.is_valid = false;
            throw HydraulicException(
                "Simulation violates physical constraints (negative pressure detected)",
                HydraulicException::ErrorCategory::PHYSICAL_CONSTRAINT_ERROR,
                "Simulator",
                current_time_
            );
        }
        
    } catch (const HydraulicException&) {
        // Increment error count and re-throw
        error_count_++;
        throw;
    } catch (const std::exception& e) {
        // Unexpected error
        error_count_++;
        current_state_.error = e.what();
        current_state_.is_valid = false;
        throw HydraulicException(
            std::string("Unexpected error during simulation step: ") + e.what(),
            HydraulicException::ErrorCategory::SIMULATION_INSTABILITY,
            "Simulator",
            current_time_
        );
    }
}

void Simulator::reset() {
    current_time_ = 0.0;
    
    if (!initialized_) {
        throw HydraulicException(
            "Cannot reset uninitialized simulator",
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator"
        );
    }
    
    if (accumulator_) accumulator_->reset(current_state_.p1);
    if (cylinder_) cylinder_->reset(0.0);
    if (fill_valve_) fill_valve_->close();
    if (drain_valve_) drain_valve_->close();
    if (relief_valve_) relief_valve_->close();
    
    // Reset error counts
    warning_count_ = 0;
    error_count_ = 0;
    
    updateState();
    current_state_.error = std::nullopt;
    current_state_.is_valid = true;
}

bool Simulator::setValveState(const std::string& valve_id, ValveState state) {
    if (!initialized_) {
        throw HydraulicException(
            "Cannot set valve state on uninitialized simulator",
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator",
            current_time_
        );
    }
    
    if (!isValidValveId(valve_id)) {
        throw HydraulicException(
            "Invalid valve ID: " + valve_id,
            HydraulicException::ErrorCategory::VALVE_OPERATION_ERROR,
            valve_id,
            current_time_
        );
    }
    
    if (valve_id == "fill_valve" && fill_valve_) {
        fill_valve_->setState(state);
        updateState();
        return true;
    } else if (valve_id == "drain_valve" && drain_valve_) {
        drain_valve_->setState(state);
        updateState();
        return true;
    } else if (valve_id == "relief_valve" && relief_valve_) {
        // Check if trying to manually control an automatic valve
        if (state != relief_valve_->getState()) {
            recordWarning(
                "Attempting to manually control an automatic relief valve",
                "relief_valve"
            );
        }
        relief_valve_->setState(state);
        updateState();
        return true;
    }
    
    return false;
}

ValveState Simulator::getValveState(const std::string& valve_id) const {
    if (!initialized_) {
        throw HydraulicException(
            "Cannot get valve state from uninitialized simulator",
            HydraulicException::ErrorCategory::INITIALIZATION_ERROR,
            "Simulator",
            current_time_
        );
    }
    
    if (!isValidValveId(valve_id)) {
        throw HydraulicException(
            "Invalid valve ID: " + valve_id,
            HydraulicException::ErrorCategory::VALVE_OPERATION_ERROR,
            valve_id,
            current_time_
        );
    }
    
    if (valve_id == "fill_valve" && fill_valve_) {
        return fill_valve_->getState();
    } else if (valve_id == "drain_valve" && drain_valve_) {
        return drain_valve_->getState();
    } else if (valve_id == "relief_valve" && relief_valve_) {
        return relief_valve_->getState();
    }
    
    // This should never happen due to the isValidValveId check
    throw HydraulicException(
        "Unknown error getting valve state",
        HydraulicException::ErrorCategory::VALVE_OPERATION_ERROR,
        valve_id,
        current_time_
    );
}

bool Simulator::checkSafetyViolation() const {
    return cylinder_ && current_state_.p2 > safety_pressure_limit_;
}

bool Simulator::checkSimulationStability() const {
    // Check for NaN or infinite values
    if (!std::isfinite(current_state_.p1) || !std::isfinite(current_state_.p2)) {
        return false;
    }
    
    // Check for extreme values that might indicate instability
    const double extreme_pressure = 1.0e9;  // 1 GPa - unrealistically high
    if (std::abs(current_state_.p1) > extreme_pressure || 
        std::abs(current_state_.p2) > extreme_pressure) {
        return false;
    }
    
    return true;
}

bool Simulator::checkPhysicalValidity() const {
    // Check for negative pressures (physical impossibility)
    if (current_state_.p1 < 0.0 || current_state_.p2 < 0.0) {
        return false;
    }
    
    // Check that pressures are within operational limits
    if (current_state_.p1 > limits_.max_pressure || 
        current_state_.p2 > limits_.max_pressure) {
        return false;
    }
    
    return true;
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

void Simulator::validateParameters(double p1, double p2) const {
    // Check for extreme or invalid pressure values
    if (!std::isfinite(p1)) {
        throw HydraulicException(
            "Accumulator pressure is not a finite value",
            HydraulicException::ErrorCategory::PHYSICAL_CONSTRAINT_ERROR,
            "Accumulator",
            current_time_
        );
    }
    
    if (!std::isfinite(p2)) {
        throw HydraulicException(
            "Cylinder pressure is not a finite value",
            HydraulicException::ErrorCategory::PHYSICAL_CONSTRAINT_ERROR,
            "Cylinder",
            current_time_
        );
    }
    
    if (p1 < limits_.min_pressure) {
        throw HydraulicException(
            "Accumulator pressure below minimum allowed: " + std::to_string(p1) + " Pa",
            HydraulicException::ErrorCategory::PHYSICAL_CONSTRAINT_ERROR,
            "Accumulator",
            current_time_
        );
    }
    
    if (p2 < limits_.min_pressure) {
        throw HydraulicException(
            "Cylinder pressure below minimum allowed: " + std::to_string(p2) + " Pa",
            HydraulicException::ErrorCategory::PHYSICAL_CONSTRAINT_ERROR,
            "Cylinder",
            current_time_
        );
    }
    
    if (p1 > limits_.max_pressure) {
        throw HydraulicException(
            "Accumulator pressure exceeds maximum allowed: " + std::to_string(p1) + " Pa",
            HydraulicException::ErrorCategory::PRESSURE_VIOLATION,
            "Accumulator",
            current_time_
        );
    }
    
    if (p2 > limits_.max_pressure) {
        throw HydraulicException(
            "Cylinder pressure exceeds maximum allowed: " + std::to_string(p2) + " Pa",
            HydraulicException::ErrorCategory::PRESSURE_VIOLATION,
            "Cylinder",
            current_time_
        );
    }
}

void Simulator::recordWarning(const std::string& message, const std::string& component) {
    warning_count_++;
    
    // Only print up to MAX_WARNINGS to avoid flooding logs
    if (warning_count_ <= MAX_WARNINGS) {
        std::cerr << "[WARNING] " << (component.empty() ? "" : component + ": ") 
                 << message << " (at t=" << current_time_ << "s)" << std::endl;
    } else if (warning_count_ == MAX_WARNINGS + 1) {
        std::cerr << "[WARNING] Additional warnings suppressed..." << std::endl;
    }
}

bool Simulator::isValidValveId(const std::string& valve_id) const {
    static const std::array<std::string, 3> valid_ids = {
        "fill_valve", "drain_valve", "relief_valve"
    };
    
    return std::find(valid_ids.begin(), valid_ids.end(), valve_id) != valid_ids.end();
}

void Simulator::checkComponentsInitialized() const {
    if (!accumulator_) {
        throw HydraulicException(
            "Accumulator component not initialized",
            HydraulicException::ErrorCategory::COMPONENT_ERROR,
            "Accumulator",
            current_time_
        );
    }
    
    if (!cylinder_) {
        throw HydraulicException(
            "Cylinder component not initialized",
            HydraulicException::ErrorCategory::COMPONENT_ERROR,
            "Cylinder",
            current_time_
        );
    }
    
    if (!fill_valve_) {
        throw HydraulicException(
            "Fill valve component not initialized",
            HydraulicException::ErrorCategory::COMPONENT_ERROR,
            "fill_valve",
            current_time_
        );
    }
    
    if (!drain_valve_) {
        throw HydraulicException(
            "Drain valve component not initialized",
            HydraulicException::ErrorCategory::COMPONENT_ERROR,
            "drain_valve",
            current_time_
        );
    }
    
    if (!relief_valve_) {
        throw HydraulicException(
            "Relief valve component not initialized",
            HydraulicException::ErrorCategory::COMPONENT_ERROR,
            "relief_valve",
            current_time_
        );
    }
}
