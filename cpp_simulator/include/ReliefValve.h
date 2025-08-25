#pragma once
#include "Orifice.h"

class ReliefValve : public Orifice {
private:
    double p_max_;
    double p_deadband_;
    bool auto_mode_;  // True for automatic operation, false for manual control
    
public:
    ReliefValve(double area, double p_max, double p_deadband, 
                ValveState initial_state = ValveState::CLOSED);
    
    // Getters
    double getPMax() const { return p_max_; }
    double getPDeadband() const { return p_deadband_; }
    bool isAutoMode() const { return auto_mode_; }
    
    // Setters
    void setPMax(double p_max) { p_max_ = p_max; }
    void setPDeadband(double p_deadband) { p_deadband_ = p_deadband; }
    void setAutoMode(bool auto_mode) { auto_mode_ = auto_mode; }
    
    // Override state control for manual mode
    void setState(ValveState state) override;
    
    // Automatic pressure relief logic
    void update(double p1, double p2, double dt) override;
};
