#include "Orifice.h"
#include <cmath>
#include <algorithm>

Orifice::Orifice(double area, ValveState initial_state)
    : area_(area), state_(initial_state) {
}

double Orifice::calculateFlow(double p1, double p2) const {
    if (state_ == ValveState::CLOSED) {
        return 0.0;
    }
    
    // Simplified orifice flow equation: Q = Cd * A * sqrt(2 * dp / rho)
    // Assuming Cd = 0.6 and rho = 850 kg/m³ (hydraulic fluid)
    const double Cd = 0.6;
    const double rho = 850.0;
    
    double dp = p1 - p2;
    if (std::abs(dp) < 1e-6) {
        return 0.0;  // No pressure difference
    }
    
    double flow = Cd * area_ * std::sqrt(2.0 * std::abs(dp) / rho);
    
    // Flow direction: positive when p1 > p2
    return (dp > 0) ? flow : -flow;
}
