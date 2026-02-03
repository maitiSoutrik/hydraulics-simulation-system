#include "gtest/gtest.h"
#include "Simulator.h"
#include "HydraulicException.h"
#include <memory>
#include <stdexcept>
#include <limits>
#include <cmath>

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default valid parameters for tests
        dt_ = 0.001;                   // 1 ms time step
        max_time_ = 10.0;              // 10 seconds simulation
        safety_pressure_ = 2.0e6;      // 2 MPa safety limit
        
        p1_initial_ = 5.0e5;           // 500 kPa accumulator pressure
        p2_initial_ = 0.0;             // 0 Pa cylinder pressure
        volume_cylinder_ = 0.001;      // 1 liter cylinder
        beta_ = 1.5e9;                 // 1.5 GPa bulk modulus (typical for hydraulic oil)
        p_max_ = 1.0e6;                // 1 MPa relief valve setting
        p_deadband_ = 1.0e5;           // 100 kPa relief valve deadband
        fill_area_ = 1.0e-5;           // 10 mm² fill valve area
        drain_area_ = 1.0e-5;          // 10 mm² drain valve area
        relief_area_ = 2.0e-5;         // 20 mm² relief valve area
    }
    
    // Default valid parameters
    double dt_;
    double max_time_;
    double safety_pressure_;
    
    double p1_initial_;
    double p2_initial_;
    double volume_cylinder_;
    double beta_;
    double p_max_;
    double p_deadband_;
    double fill_area_;
    double drain_area_;
    double relief_area_;
    
    // Helper to create a simulator with standard parameters
    std::unique_ptr<Simulator> createDefaultSimulator() {
        return std::make_unique<Simulator>(dt_, max_time_, safety_pressure_);
    }
    
    // Initialize with default parameters
    void initializeDefault(Simulator& simulator) {
        simulator.initialize(
            p1_initial_, p2_initial_, volume_cylinder_,
            beta_, p_max_, p_deadband_,
            fill_area_, drain_area_, relief_area_
        );
    }
};

// Test constructor parameter validation
TEST_F(ErrorHandlingTest, ConstructorParameterValidation) {
    // Test negative time step
    EXPECT_THROW({
        Simulator sim(-0.001, max_time_, safety_pressure_);
    }, HydraulicException);
    
    // Test negative max time
    EXPECT_THROW({
        Simulator sim(dt_, -1.0, safety_pressure_);
    }, HydraulicException);
    
    // Test negative safety pressure
    EXPECT_THROW({
        Simulator sim(dt_, max_time_, -1000.0);
    }, HydraulicException);
    
    // Test zero safety pressure
    EXPECT_THROW({
        Simulator sim(dt_, max_time_, 0.0);
    }, HydraulicException);
    
    // Test valid parameters
    EXPECT_NO_THROW({
        Simulator sim(dt_, max_time_, safety_pressure_);
    });
}

// Test initialization parameter validation
TEST_F(ErrorHandlingTest, InitializationParameterValidation) {
    auto sim = createDefaultSimulator();
    
    // Test negative initial pressures
    EXPECT_THROW({
        sim->initialize(-1.0, p2_initial_, volume_cylinder_, beta_, p_max_, p_deadband_,
                       fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, -1.0, volume_cylinder_, beta_, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    // Test negative/zero volume
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, -1.0, beta_, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, 0.0, beta_, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    // Test negative/zero bulk modulus
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, -1.0, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, 0.0, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    // Test negative/zero relief valve max pressure
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, -1.0, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, 0.0, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    // Test negative deadband
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, p_max_, -1.0,
                      fill_area_, drain_area_, relief_area_);
    }, HydraulicException);
    
    // Test negative valve areas
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, p_max_, p_deadband_,
                      -1.0, drain_area_, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, p_max_, p_deadband_,
                      fill_area_, -1.0, relief_area_);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, p_max_, p_deadband_,
                      fill_area_, drain_area_, -1.0);
    }, HydraulicException);
    
    // Test valid parameters
    EXPECT_NO_THROW({
        sim->initialize(p1_initial_, p2_initial_, volume_cylinder_, beta_, p_max_, p_deadband_,
                      fill_area_, drain_area_, relief_area_);
    });
}

// Test valve operation error handling
TEST_F(ErrorHandlingTest, ValveOperationErrorHandling) {
    auto sim = createDefaultSimulator();
    initializeDefault(*sim);
    
    // Test invalid valve ID
    EXPECT_THROW({
        sim->setValveState("non_existent_valve", ValveState::OPEN);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->getValveState("non_existent_valve");
    }, HydraulicException);
    
    // Test valid valve IDs
    EXPECT_NO_THROW({
        sim->setValveState("fill_valve", ValveState::OPEN);
    });
    
    EXPECT_NO_THROW({
        sim->getValveState("fill_valve");
    });
    
    EXPECT_NO_THROW({
        sim->setValveState("drain_valve", ValveState::OPEN);
    });
    
    EXPECT_NO_THROW({
        sim->getValveState("drain_valve");
    });
    
    EXPECT_NO_THROW({
        sim->setValveState("relief_valve", ValveState::OPEN);
    });
    
    EXPECT_NO_THROW({
        sim->getValveState("relief_valve");
    });
}

// Test safety violation detection
TEST_F(ErrorHandlingTest, SafetyViolationDetection) {
    auto sim = createDefaultSimulator();
    
    // Initialize with p2_initial close to safety limit
    double close_to_limit = safety_pressure_ - 1000.0;  // Just below limit
    
    sim->initialize(
        p1_initial_, close_to_limit, volume_cylinder_,
        beta_, p_max_, p_deadband_,
        fill_area_, drain_area_, relief_area_
    );
    
    // Open fill valve to cause pressure to rise
    sim->setValveState("fill_valve", ValveState::OPEN);
    
    // Run until safety violation or timeout
    bool safety_violation_detected = false;
    double timeout = 1.0;  // 1 second timeout
    
    try {
        for (int i = 0; i < static_cast<int>(timeout / dt_); i++) {
            sim->step();
        }
    } catch (const HydraulicException& e) {
        // Check if this is a pressure violation exception
        safety_violation_detected = 
            (e.getCategory() == HydraulicException::ErrorCategory::PRESSURE_VIOLATION);
    }
    
    EXPECT_TRUE(safety_violation_detected);
    EXPECT_TRUE(sim->getState().error.has_value());
    EXPECT_FALSE(sim->getState().is_valid);
}

// Test error category string conversion
TEST_F(ErrorHandlingTest, ErrorCategoryStringConversion) {
    // Test that exceptions have proper formatted error messages
    try {
        throw HydraulicException(
            "Test error message",
            HydraulicException::ErrorCategory::PRESSURE_VIOLATION,
            "TestComponent",
            123.45
        );
    } catch (const HydraulicException& e) {
        std::string error_message = e.what();
        
        // Check that the message contains the category
        EXPECT_TRUE(error_message.find("PRESSURE_VIOLATION") != std::string::npos);
        
        // Check that the message contains the component
        EXPECT_TRUE(error_message.find("TestComponent") != std::string::npos);
        
        // Check that the message contains the time
        EXPECT_TRUE(error_message.find("123.45") != std::string::npos);
        
        // Check that the message contains the original message
        EXPECT_TRUE(error_message.find("Test error message") != std::string::npos);
    }
}

// Test operational limits validation
TEST_F(ErrorHandlingTest, OperationalLimitsValidation) {
    auto sim = createDefaultSimulator();
    
    SimulationLimits valid_limits;
    valid_limits.max_pressure = 5.0e6;       // 5 MPa
    valid_limits.min_pressure = 0.0;         // 0 Pa
    valid_limits.max_pressure_rate = 1.0e6;  // 1 MPa/s
    valid_limits.max_flow_rate = 0.01;       // 10 L/s
    valid_limits.min_bulk_modulus = 1.0e6;   // 1 MPa
    valid_limits.max_time_step = 0.01;       // 10 ms
    
    // Test valid limits
    EXPECT_NO_THROW({
        sim->setOperationalLimits(valid_limits);
    });
    
    // Test negative max pressure
    SimulationLimits invalid_limits1 = valid_limits;
    invalid_limits1.max_pressure = -1.0;
    EXPECT_THROW({
        sim->setOperationalLimits(invalid_limits1);
    }, HydraulicException);
    
    // Test min > max pressure
    SimulationLimits invalid_limits2 = valid_limits;
    invalid_limits2.min_pressure = 6.0e6;  // > max_pressure
    EXPECT_THROW({
        sim->setOperationalLimits(invalid_limits2);
    }, HydraulicException);
    
    // Test negative max flow rate
    SimulationLimits invalid_limits3 = valid_limits;
    invalid_limits3.max_flow_rate = -0.01;
    EXPECT_THROW({
        sim->setOperationalLimits(invalid_limits3);
    }, HydraulicException);
    
    // Test negative min bulk modulus
    SimulationLimits invalid_limits4 = valid_limits;
    invalid_limits4.min_bulk_modulus = -1.0e6;
    EXPECT_THROW({
        sim->setOperationalLimits(invalid_limits4);
    }, HydraulicException);
    
    // Test negative max time step
    SimulationLimits invalid_limits5 = valid_limits;
    invalid_limits5.max_time_step = -0.01;
    EXPECT_THROW({
        sim->setOperationalLimits(invalid_limits5);
    }, HydraulicException);
}

// Test uninitialized simulator operations
TEST_F(ErrorHandlingTest, UninitializedSimulatorOperations) {
    auto sim = createDefaultSimulator();
    
    // Operations on uninitialized simulator should throw
    EXPECT_THROW({
        sim->step();
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->reset();
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->setValveState("fill_valve", ValveState::OPEN);
    }, HydraulicException);
    
    EXPECT_THROW({
        sim->getValveState("fill_valve");
    }, HydraulicException);
    
    // After initialization, operations should work
    initializeDefault(*sim);
    
    EXPECT_NO_THROW({
        sim->reset();
    });
    
    EXPECT_NO_THROW({
        sim->setValveState("fill_valve", ValveState::OPEN);
    });
    
    EXPECT_NO_THROW({
        sim->getValveState("fill_valve");
    });
    
    EXPECT_NO_THROW({
        sim->step();
    });
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}