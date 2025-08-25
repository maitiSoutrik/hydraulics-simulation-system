#include "ReliefValve.h"

ReliefValve::ReliefValve(double area, double p_max, double p_deadband, 
                         ValveState initial_state)
    : Orifice(area, initial_state), p_max_(p_max), p_deadband_(p_deadband), 
      auto_mode_(true) {
}

void ReliefValve::setState(ValveState state) {
    if (!auto_mode_) {
        // Manual control - allow direct state setting
        state_ = state;
    }
    // In auto mode, ignore manual state changes
}

void ReliefValve::update(double p1, double p2, double dt) {
    if (!auto_mode_) {
        return;  // Manual control mode
    }
    
    // Automatic pressure relief logic
    // The upstream pressure is p1, downstream is p2
    // Open when p1 exceeds P_max, close when p1 drops below (P_max - P_deadband)
    
    if (state_ == ValveState::CLOSED && p1 > p_max_) {
        state_ = ValveState::OPEN;
    } else if (state_ == ValveState::OPEN && p1 < (p_max_ - p_deadband_)) {
        state_ = ValveState::CLOSED;
    }
}
