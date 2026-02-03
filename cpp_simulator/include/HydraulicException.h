#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

/**
 * @class HydraulicException
 * @brief Custom exception class for hydraulic simulation errors
 * 
 * This exception class provides context-rich information about errors
 * in the hydraulic simulation system, including component information,
 * error type categorization, and simulation state at time of error.
 */
class HydraulicException : public std::runtime_error {
public:
    /**
     * @brief Error category enumeration for hydraulic system errors
     */
    enum class ErrorCategory {
        INITIALIZATION_ERROR,       ///< Error during system initialization
        PARAMETER_ERROR,            ///< Invalid parameter values
        SIMULATION_INSTABILITY,     ///< Simulation became unstable
        VALVE_OPERATION_ERROR,      ///< Error in valve operation
        PRESSURE_VIOLATION,         ///< Pressure exceeds safety limits
        PHYSICAL_CONSTRAINT_ERROR,  ///< Physical laws violated (e.g., negative pressure)
        COMPONENT_ERROR             ///< Error in a specific component
    };

private:
    ErrorCategory category_;        ///< Error category
    std::string component_;         ///< Component where error occurred
    double simulation_time_;        ///< Simulation time when error occurred
    
    /**
     * @brief Format the error message with additional context
     * @param base_message Basic error message
     * @return Formatted error message with context
     */
    static std::string formatMessage(
        const std::string& base_message,
        const std::string& component,
        ErrorCategory category,
        double simulation_time) {
        
        std::ostringstream oss;
        oss << "[" << categoryToString(category) << "] ";
        
        if (!component.empty()) {
            oss << "Component: " << component << " | ";
        }
        
        oss << "Time: " << simulation_time << "s | ";
        oss << base_message;
        
        return oss.str();
    }
    
    /**
     * @brief Convert error category to string representation
     * @param category Error category
     * @return String representation of error category
     */
    static std::string categoryToString(ErrorCategory category) {
        switch (category) {
            case ErrorCategory::INITIALIZATION_ERROR: return "INIT_ERROR";
            case ErrorCategory::PARAMETER_ERROR: return "PARAM_ERROR";
            case ErrorCategory::SIMULATION_INSTABILITY: return "INSTABILITY";
            case ErrorCategory::VALVE_OPERATION_ERROR: return "VALVE_ERROR";
            case ErrorCategory::PRESSURE_VIOLATION: return "PRESSURE_VIOLATION";
            case ErrorCategory::PHYSICAL_CONSTRAINT_ERROR: return "PHYSICS_ERROR";
            case ErrorCategory::COMPONENT_ERROR: return "COMPONENT_ERROR";
            default: return "UNKNOWN_ERROR";
        }
    }

public:
    /**
     * @brief Constructor for hydraulic exception
     * @param message Error message
     * @param category Error category
     * @param component Component where error occurred (empty if not component-specific)
     * @param time Simulation time when error occurred
     */
    HydraulicException(
        const std::string& message,
        ErrorCategory category,
        const std::string& component = "",
        double time = 0.0
    ) : std::runtime_error(formatMessage(message, component, category, time)),
        category_(category),
        component_(component),
        simulation_time_(time) {}
    
    /**
     * @brief Get error category
     * @return Error category
     */
    ErrorCategory getCategory() const { return category_; }
    
    /**
     * @brief Get component name
     * @return Component name
     */
    const std::string& getComponent() const { return component_; }
    
    /**
     * @brief Get simulation time
     * @return Simulation time
     */
    double getSimulationTime() const { return simulation_time_; }
};