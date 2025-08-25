#include <gtest/gtest.h>
#include "ReliefValve.h"

class ReliefValveTest : public ::testing::Test {
protected:
    void SetUp() override {
        double p_max = 1200.0;
        double p_deadband = 50.0;
        valve = std::make_unique<ReliefValve>(1e-5, p_max, p_deadband, ValveState::CLOSED);
    }
    
    std::unique_ptr<ReliefValve> valve;
};

TEST_F(ReliefValveTest, InitialState) {
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
    EXPECT_DOUBLE_EQ(valve->getArea(), 1e-5);
    EXPECT_DOUBLE_EQ(valve->getPMax(), 1200.0);
    EXPECT_DOUBLE_EQ(valve->getPDeadband(), 50.0);
    EXPECT_TRUE(valve->isAutoMode());
}

TEST_F(ReliefValveTest, AutomaticOpening) {
    double p1 = 1000.0;
    double p2 = 1250.0;  // Above P_max
    double dt = 0.01;
    
    // Initially closed
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
    
    // Update should open valve
    valve->update(p1, p2, dt);
    EXPECT_EQ(valve->getState(), ValveState::OPEN);
}

TEST_F(ReliefValveTest, AutomaticClosing) {
    double p1 = 1000.0;
    double p2_high = 1250.0;  // Above P_max
    double p2_low = 1140.0;   // Below (P_max - P_deadband)
    double dt = 0.01;
    
    // First open the valve
    valve->update(p1, p2_high, dt);
    EXPECT_EQ(valve->getState(), ValveState::OPEN);
    
    // Then close it with low pressure
    valve->update(p1, p2_low, dt);
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
}

TEST_F(ReliefValveTest, ManualMode) {
    valve->setAutoMode(false);
    EXPECT_FALSE(valve->isAutoMode());
    
    // Manual control should work
    valve->setState(ValveState::OPEN);
    EXPECT_EQ(valve->getState(), ValveState::OPEN);
    
    // Auto update should not affect manual mode
    double p1 = 1000.0;
    double p2 = 1250.0;  // Above P_max
    double dt = 0.01;
    
    valve->setState(ValveState::CLOSED);
    valve->update(p1, p2, dt);  // This should not open valve in manual mode
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
}
