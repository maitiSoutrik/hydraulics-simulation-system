#pragma once
#include "Volume.h"
#include "FillValve.h"
#include "ReliefValve.h"
#include "HydraulicException.h"
#include <memory>
#include <string>
#include <map>
#include <limits>
#include <optional>

/**
 * @struct SimulationState
 * @brief Container for the current state of the hydraulic simulation
 */
struct SimulationState {
    double time;                              ///< Current simulation time
    double p1;                                ///< Accumulator pressure
    double p2;                                ///< Cylinder pressure
    std::map<std::string, ValveState> valve_states;  ///< States of all valves
    
    // Error state tracking
    std::optional<std::string> error;         ///< Error message if simulation is in error state
    bool is_valid = true;                     ///< Simulation validity flag
};

/**
 * @struct SimulationLimits
 * @brief Defines operational limits and safety thresholds for the simulation
 */
struct SimulationLimits {
    double max_pressure;            ///< Maximum allowable pressure (Pa)
    double min_pressure = 0.0;      ///< Minimum allowable pressure (Pa)
    double max_pressure_rate;       ///< Maximum allowable pressure change rate (Pa/s)
    double max_flow_rate;           ///< Maximum allowable flow rate (m³/s)
    double min_bulk_modulus;        ///< Minimum allowable bulk modulus (Pa)
    double max_time_step;           ///< Maximum allowable time step (s)
};

/**
 * @class Simulator
 * @brief Core hydraulic system simulator with enhanced error handling
 * 
 * This class simulates a hydraulic system with an accumulator, cylinder, and valves.
 * It includes comprehensive error detection, validation, and safety monitoring.
 */
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
    bool initialized_ = false;
    
    // Safety limits
    SimulationLimits limits_;
    
    // Error tracking
    unsigned int warning_count_ = 0;
    unsigned int error_count_ = 0;
    static constexpr unsigned int MAX_WARNINGS = 10;
    
public:
    /**
     * @brief Construct a new Simulator object
     * 
     * @param dt Time step for simulation (seconds)
     * @param max_time Maximum simulation time (seconds)
     * @param safety_pressure_limit Maximum safe pressure (Pa)
     * @throws HydraulicException if parameters are invalid
     */
    Simulator(double dt, double max_time, double safety_pressure_limit);
    
    /**
     * @brief Set operational limits for the simulation
     * 
     * @param limits Struct containing simulation limits
     * @throws HydraulicException if limits are physically invalid
     */
    void setOperationalLimits(const SimulationLimits& limits);
    
    /**
     * @brief Initialize the hydraulic system with specified parameters
     * 
     * @param p1_initial Initial accumulator pressure (Pa)
     * @param p2_initial Initial cylinder pressure (Pa)
     * @param volume_cylinder Cylinder volume (m³)
     * @param beta Bulk modulus of hydraulic fluid (Pa)
     * @param p_max Maximum pressure for relief valve (Pa)
     * @param p_deadband Pressure deadband for relief valve (Pa)
     * @param fill_area Fill valve orifice area (m²)
     * @param drain_area Drain valve orifice area (m²)
     * @param relief_area Relief valve orifice area (m²)
     * @throws HydraulicException if parameters are invalid or initialization fails
     */
    void initialize(double p1_initial, double p2_initial, double volume_cylinder, 
                   double beta, double p_max, double p_deadband,
                   double fill_area, double drain_area, double relief_area);
    
    /**
     * @brief Advance the simulation by one time step
     * 
     * @throws HydraulicException if simulation becomes unstable or physically invalid
     */
    void step();
    
    /**
     * @brief Reset the simulation to initial conditions
     */
    void reset();
    
    /**
     * @brief Check if simulation has reached end time
     * 
     * @return true if simulation is complete
     */
    bool isFinished() const { return current_time_ >= max_time_; }
    
    /**
     * @brief Get current simulation state
     * 
     * @return const reference to current SimulationState
     */
    const SimulationState& getState() const { return current_state_; }
    
    /**
     * @brief Get current simulation time
     * 
     * @return current time (seconds)
     */
    double getCurrentTime() const { return current_time_; }
    
    /**
     * @brief Set the state of a specific valve
     * 
     * @param valve_id Valve identifier ("fill_valve", "drain_valve", "relief_valve")
     * @param state Desired valve state (OPEN or CLOSED)
     * @return true if valve state was successfully set
     * @throws HydraulicException if valve_id is invalid
     */
    bool setValveState(const std::string& valve_id, ValveState state);
    
    /**
     * @brief Get current state of a specific valve
     * 
     * @param valve_id Valve identifier
     * @return Current valve state
     * @throws HydraulicException if valve_id is invalid
     */
    ValveState getValveState(const std::string& valve_id) const;
    
    /**
     * @brief Check if simulation exceeds safety pressure limit
     * 
     * @return true if safety violation detected
     */
    bool checkSafetyViolation() const;
    
    /**
     * @brief Check if simulation is numerically stable
     * 
     * @return true if simulation is stable
     */
    bool checkSimulationStability() const;
    
    /**
     * @brief Check if simulation adheres to physical constraints
     * 
     * @return true if physically valid
     */
    bool checkPhysicalValidity() const;
    
    /**
     * @brief Get number of warnings encountered
     * 
     * @return Warning count
     */
    unsigned int getWarningCount() const { return warning_count_; }
    
    /**
     * @brief Get number of errors encountered (and recovered from)
     * 
     * @return Error count
     */
    unsigned int getErrorCount() const { return error_count_; }
    
private:
    /**
     * @brief Update the simulation state
     */
    void updateState();
    
    /**
     * @brief Convert valve state to string representation
     * 
     * @param state Valve state
     * @return String representation
     */
    std::string getValveStateString(ValveState state) const;
    
    /**
     * @brief Validate simulation parameters
     * 
     * @param p1 Accumulator pressure
     * @param p2 Cylinder pressure
     * @throws HydraulicException if parameters are invalid
     */
    void validateParameters(double p1, double p2) const;
    
    /**
     * @brief Record a warning condition
     * 
     * @param message Warning message
     * @param component Affected component
     */
    void recordWarning(const std::string& message, const std::string& component = "");
    
    /**
     * @brief Check if valve ID is valid
     * 
     * @param valve_id Valve identifier to check
     * @return true if valve ID is valid
     */
    bool isValidValveId(const std::string& valve_id) const;
    
    /**
     * @brief Check if component pointers are valid
     * 
     * @throws HydraulicException if any component is null
     */
    void checkComponentsInitialized() const;
};
