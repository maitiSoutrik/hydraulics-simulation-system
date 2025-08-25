#include "FillValve.h"
#include <algorithm>

FillValve::FillValve(double area, ValveState initial_state)
    : Orifice(area, initial_state) {
}

double FillValve::calculateFlow(double p1, double p2) const {
    if (state_ == ValveState::CLOSED) {
        return 0.0;
    }
    
    // Calculate base flow using parent method
    double flow = Orifice::calculateFlow(p1, p2);
    
    // Prevent backflow: only allow flow when p1 > p2
    return std::max(0.0, flow);
}
