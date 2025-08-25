#pragma once

class Volume {
private:
    double pressure_;
    double volume_;
    double beta_;  // Bulk modulus

public:
    Volume(double initial_pressure, double volume, double beta);
    
    // Getters
    double getPressure() const { return pressure_; }
    double getVolume() const { return volume_; }
    double getBeta() const { return beta_; }
    
    // Update pressure based on flow rate (positive = inflow, negative = outflow)
    void updatePressure(double flow_rate, double dt);
    
    // Reset to initial conditions
    void reset(double initial_pressure);
};
