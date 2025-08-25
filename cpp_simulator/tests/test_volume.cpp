#include <gtest/gtest.h>
#include "Volume.h"

class VolumeTest : public ::testing::Test {
protected:
    void SetUp() override {
        volume = std::make_unique<Volume>(1000.0, 0.001, 2.1e9);
    }
    
    std::unique_ptr<Volume> volume;
};

TEST_F(VolumeTest, InitialConditions) {
    EXPECT_DOUBLE_EQ(volume->getPressure(), 1000.0);
    EXPECT_DOUBLE_EQ(volume->getVolume(), 0.001);
    EXPECT_DOUBLE_EQ(volume->getBeta(), 2.1e9);
}

TEST_F(VolumeTest, PressureUpdate) {
    double initial_pressure = volume->getPressure();
    double flow_rate = 1e-6;  // 1 ml/s inflow
    double dt = 0.01;
    
    volume->updatePressure(flow_rate, dt);
    
    // Pressure should increase with positive flow
    EXPECT_GT(volume->getPressure(), initial_pressure);
}

TEST_F(VolumeTest, NegativePressurePrevention) {
    // Try to create large negative pressure
    double large_outflow = -1.0;  // Very large outflow
    double dt = 0.01;
    
    volume->updatePressure(large_outflow, dt);
    
    // Pressure should not go below zero
    EXPECT_GE(volume->getPressure(), 0.0);
}

TEST_F(VolumeTest, Reset) {
    // Change pressure
    volume->updatePressure(1e-6, 0.01);
    double changed_pressure = volume->getPressure();
    
    // Reset to new value
    double new_pressure = 500.0;
    volume->reset(new_pressure);
    
    EXPECT_DOUBLE_EQ(volume->getPressure(), new_pressure);
    EXPECT_NE(volume->getPressure(), changed_pressure);
}
