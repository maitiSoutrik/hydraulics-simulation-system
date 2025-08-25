#include <gtest/gtest.h>
#include "Simulator.h"

class SimulatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        double dt = 0.01;
        double max_time = 10.0;
        double safety_limit = 1500.0;
        
        simulator = std::make_unique<Simulator>(dt, max_time, safety_limit);
        
        // Initialize with typical values
        simulator->initialize(
            1000.0,  // p1_initial
            0.0,     // p2_initial
            0.001,   // volume_cylinder
            2.1e9,   // beta
            1200.0,  // p_max
            50.0,    // p_deadband
            1e-5,    // fill_area
            1e-5,    // drain_area
            1e-5     // relief_area
        );
    }
    
    std::unique_ptr<Simulator> simulator;
};

TEST_F(SimulatorTest, InitialConditions) {
    const auto& state = simulator->getState();
    
    EXPECT_DOUBLE_EQ(state.time, 0.0);
    EXPECT_DOUBLE_EQ(state.p1, 1000.0);
    EXPECT_DOUBLE_EQ(state.p2, 0.0);
    EXPECT_FALSE(simulator->isFinished());
}

TEST_F(SimulatorTest, ValveControl) {
    // Test setting valve states
    EXPECT_TRUE(simulator->setValveState("fill_valve", ValveState::OPEN));
    EXPECT_EQ(simulator->getValveState("fill_valve"), ValveState::OPEN);
    
    EXPECT_TRUE(simulator->setValveState("fill_valve", ValveState::CLOSED));
    EXPECT_EQ(simulator->getValveState("fill_valve"), ValveState::CLOSED);
    
    // Test invalid valve ID
    EXPECT_FALSE(simulator->setValveState("invalid_valve", ValveState::OPEN));
}

TEST_F(SimulatorTest, SimulationStep) {
    double initial_time = simulator->getCurrentTime();
    
    simulator->step();
    
    // Time should advance
    EXPECT_GT(simulator->getCurrentTime(), initial_time);
}

TEST_F(SimulatorTest, SafetyChecks) {
    // Initially should be safe
    EXPECT_TRUE(simulator->checkSimulationStability());
    EXPECT_TRUE(simulator->checkPhysicalValidity());
    EXPECT_FALSE(simulator->checkSafetyViolation());
}
