# Enhanced Error Handling for Hydraulics Simulation System

This document describes the enhanced error handling features added to the hydraulics simulation system, focusing on embedded systems best practices and safety-critical operation considerations.

## Overview

The enhanced error handling system provides:

1. **Detailed Error Context**: Each error includes component information, simulation time, and error categorization
2. **Parameter Validation**: Robust validation for all input parameters with detailed error messages
3. **Safety Monitoring**: Detection of simulation instability, physical constraint violations, and safety pressure limits
4. **Recovery Mechanisms**: State tracking and error recovery procedures
5. **Warning System**: Non-critical issues tracked and reported without halting simulation

## Key Components

### HydraulicException Class

A custom exception class that extends `std::runtime_error` with additional context for simulation errors:

- **Error Categories**: Classification of different error types (initialization, parameter, instability, etc.)
- **Component Information**: Identification of the component where the error occurred
- **Simulation Time**: Timestamp indicating when the error occurred
- **Formatted Messages**: Human-readable messages with rich context

Example:
```cpp
// Instead of a generic exception:
throw std::runtime_error("Pressure too high");

// We now have a detailed, contextual exception:
throw HydraulicException(
    "Cylinder pressure exceeds maximum allowed: 2.5e6 Pa",
    HydraulicException::ErrorCategory::PRESSURE_VIOLATION,
    "Cylinder",
    current_time_
);
```

### Enhanced SimulationState

The `SimulationState` structure now includes error tracking information:

```cpp
struct SimulationState {
    double time;
    double p1;  // Accumulator pressure
    double p2;  // Cylinder pressure
    std::map<std::string, ValveState> valve_states;
    
    // Error state tracking
    std::optional<std::string> error;  // Error message if simulation is in error state
    bool is_valid = true;              // Simulation validity flag
};
```

### Simulation Limits

A new `SimulationLimits` structure defines operational boundaries:

```cpp
struct SimulationLimits {
    double max_pressure;          // Maximum allowable pressure
    double min_pressure = 0.0;    // Minimum allowable pressure
    double max_pressure_rate;     // Maximum allowable pressure change rate
    double max_flow_rate;         // Maximum allowable flow rate
    double min_bulk_modulus;      // Minimum allowable bulk modulus
    double max_time_step;         // Maximum allowable time step
};
```

## Error Handling Improvements

### 1. Input Parameter Validation

All constructor and initialization parameters are now validated:
- Time step must be positive
- Volume must be positive
- Pressures must be non-negative
- Bulk modulus must be positive and realistic
- Valve areas must be non-negative

### 2. Runtime Error Detection

The simulator now detects and handles various runtime errors:
- Numerical instability (NaN, infinity, extreme values)
- Physical constraint violations (negative pressures, etc.)
- Safety pressure limit violations
- Component failures or null pointers

### 3. Operation Status Tracking

The simulator tracks:
- Initialization status
- Error and warning counts
- Simulation validity

### 4. Error Recovery

The enhanced system includes mechanisms for:
- Graceful failure with detailed context
- State preservation after errors
- Reset capabilities for recovering from error states

## Implementation Details

### Validation Methods

The system includes multiple validation functions:
- `validateParameters()`: Checks for valid pressure values
- `checkSimulationStability()`: Detects numerical instability
- `checkPhysicalValidity()`: Ensures physical constraints are met
- `checkComponentsInitialized()`: Verifies component initialization
- `isValidValveId()`: Validates valve identifier strings

### Error Handling Pattern

The general pattern for operations:

1. **Validate inputs**: Check all parameters before use
2. **Verify preconditions**: Ensure the system is in a valid state
3. **Perform operation**: Execute the requested action
4. **Check constraints**: Verify physical and safety constraints are met
5. **Update state**: Record the new system state
6. **Handle errors**: Catch, contextualize, and propagate exceptions

### Warning System

Non-critical issues are recorded using the `recordWarning()` method, which:
- Increments the warning counter
- Logs the warning message with component and time information
- Limits warning output to prevent log flooding

## Best Practices Implemented

1. **Fail Early, Fail Loudly**: Validate parameters at construction/initialization time
2. **Rich Context in Errors**: Include what, where, when, and why in error messages
3. **State Tracking**: Maintain knowledge of system validity
4. **Categorized Errors**: Different error types for different handling strategies
5. **Bounds Checking**: Ensure all values remain within physical and safety limits
6. **Memory Safety**: Null pointer checks and exception-safe memory handling
7. **Recovery Mechanisms**: Clean reset capabilities after error conditions

## Integration with Test Framework

A comprehensive test suite (`test_error_handling.cpp`) validates the error handling:
- Constructor parameter validation tests
- Initialization parameter validation tests
- Valve operation error tests
- Safety violation detection tests
- Error category and formatting tests
- Operational limits validation tests
- Uninitialized simulator operation tests

## Usage Examples

### Setting Operational Limits

```cpp
SimulationLimits limits;
limits.max_pressure = 5.0e6;       // 5 MPa
limits.min_pressure = 0.0;         // 0 Pa
limits.max_pressure_rate = 1.0e6;  // 1 MPa/s
limits.max_flow_rate = 0.01;       // 10 L/s
limits.min_bulk_modulus = 1.0e6;   // 1 MPa
limits.max_time_step = 0.01;       // 10 ms

simulator.setOperationalLimits(limits);
```

### Handling Exceptions

```cpp
try {
    simulator.step();
} catch (const HydraulicException& e) {
    if (e.getCategory() == HydraulicException::ErrorCategory::PRESSURE_VIOLATION) {
        // Handle pressure violation
        std::cerr << "Pressure violation in " << e.getComponent() 
                  << " at time " << e.getSimulationTime() << "s" << std::endl;
    } else if (e.getCategory() == HydraulicException::ErrorCategory::SIMULATION_INSTABILITY) {
        // Handle instability
        std::cerr << "Simulation became unstable" << std::endl;
    }
    
    // Common recovery
    simulator.reset();
}
```

### Checking Simulation State

```cpp
// After each step, check validity
simulator.step();
if (!simulator.getState().is_valid) {
    std::cout << "Simulation entered invalid state: " 
              << simulator.getState().error.value_or("Unknown error") << std::endl;
    simulator.reset();
}
```

## Benefits for Safety-Critical Systems

These error handling improvements provide several benefits for safety-critical embedded systems:

1. **Early Detection**: Problems are detected before they cause system failure
2. **Detailed Diagnostics**: Rich error context aids troubleshooting
3. **Graceful Degradation**: System can respond appropriately to different error types
4. **Error Isolation**: Errors are confined to specific components
5. **Validation**: Comprehensive test suite verifies error handling behavior
6. **Auditability**: Error tracking provides a history of system issues

## Conclusion

The enhanced error handling system significantly improves the reliability and safety of the hydraulics simulation system. By following embedded systems best practices, we've created a robust framework that detects, reports, and manages errors in a way that aligns with safety-critical system requirements.