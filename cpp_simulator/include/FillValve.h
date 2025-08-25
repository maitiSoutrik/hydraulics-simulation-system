#pragma once
#include "Orifice.h"

class FillValve : public Orifice {
public:
    FillValve(double area, ValveState initial_state = ValveState::CLOSED);
    
    // Override flow calculation to prevent backflow
    double calculateFlow(double p1, double p2) const override;
};
