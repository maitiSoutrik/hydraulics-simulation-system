#pragma once

enum class ValveState {
    OPEN,
    CLOSED
};

class Orifice {
protected:
    double area_;
    ValveState state_;
    
public:
    Orifice(double area, ValveState initial_state = ValveState::CLOSED);
    virtual ~Orifice() = default;
    
    // Getters
    double getArea() const { return area_; }
    ValveState getState() const { return state_; }
    bool isOpen() const { return state_ == ValveState::OPEN; }
    
    // State control
    virtual void setState(ValveState state) { state_ = state; }
    virtual void open() { setState(ValveState::OPEN); }
    virtual void close() { setState(ValveState::CLOSED); }
    
    // Calculate flow rate through orifice (positive = p1 to p2)
    virtual double calculateFlow(double p1, double p2) const;
    
    // Update valve state based on system conditions (for automatic valves)
    virtual void update(double p1, double p2, double dt) {}
};
