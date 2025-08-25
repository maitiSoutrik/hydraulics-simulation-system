#include "Volume.h"
#include <algorithm>
#include <cmath>

Volume::Volume(double initial_pressure, double volume, double beta)
    : pressure_(initial_pressure), volume_(volume), beta_(beta) {
}

void Volume::updatePressure(double flow_rate, double dt) {
    // dp/dt = (beta / V) * Q
    // where Q is volumetric flow rate (positive = inflow)
    double dp_dt = (beta_ / volume_) * flow_rate;
    pressure_ += dp_dt * dt;
    
    // Ensure pressure doesn't go negative
    pressure_ = std::max(0.0, pressure_);
}

void Volume::reset(double initial_pressure) {
    pressure_ = initial_pressure;
}
