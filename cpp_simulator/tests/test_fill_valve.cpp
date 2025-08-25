#include <gtest/gtest.h>
#include "FillValve.h"

class FillValveTest : public ::testing::Test {
protected:
    void SetUp() override {
        valve = std::make_unique<FillValve>(1e-5, ValveState::CLOSED);
    }
    
    std::unique_ptr<FillValve> valve;
};

TEST_F(FillValveTest, InitialState) {
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
    EXPECT_DOUBLE_EQ(valve->getArea(), 1e-5);
    EXPECT_FALSE(valve->isOpen());
}

TEST_F(FillValveTest, StateControl) {
    valve->open();
    EXPECT_EQ(valve->getState(), ValveState::OPEN);
    EXPECT_TRUE(valve->isOpen());
    
    valve->close();
    EXPECT_EQ(valve->getState(), ValveState::CLOSED);
    EXPECT_FALSE(valve->isOpen());
}

TEST_F(FillValveTest, NoFlowWhenClosed) {
    double p1 = 1000.0;
    double p2 = 500.0;
    
    // Valve is closed by default
    double flow = valve->calculateFlow(p1, p2);
    EXPECT_DOUBLE_EQ(flow, 0.0);
}

TEST_F(FillValveTest, ForwardFlowWhenOpen) {
    double p1 = 1000.0;
    double p2 = 500.0;
    
    valve->open();
    double flow = valve->calculateFlow(p1, p2);
    
    // Should have positive flow when p1 > p2
    EXPECT_GT(flow, 0.0);
}

TEST_F(FillValveTest, BackflowPrevention) {
    double p1 = 500.0;  // Lower pressure
    double p2 = 1000.0; // Higher pressure
    
    valve->open();
    double flow = valve->calculateFlow(p1, p2);
    
    // Should prevent backflow (no negative flow)
    EXPECT_EQ(flow, 0.0);
}
