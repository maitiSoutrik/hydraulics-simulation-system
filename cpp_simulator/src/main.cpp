#include "Simulator.h"
#include "ConfigLoader.h"
#include "UDPServer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Load configuration
        std::cout << "Loading configuration from: " << argv[1] << std::endl;
        SimulationConfig config = ConfigLoader::loadFromFile(argv[1]);
        
        // Create simulator
        Simulator simulator(config.dt, config.max_time, config.safety_pressure_limit);
        simulator.initialize(config.p1_initial, config.p2_initial, config.volume_cylinder,
                           config.beta, config.P_max, config.P_deadband,
                           config.fill_valve_area, config.drain_valve_area, config.relief_valve_area);
        
        // Create UDP server
        UDPServer udp_server(config.udp_port, config.telemetry_rate_hz);
        if (!udp_server.start()) {
            std::cerr << "Failed to start UDP server" << std::endl;
            return 1;
        }
        
        std::cout << "Hydraulics Simulator started successfully!" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  dt: " << config.dt << " s" << std::endl;
        std::cout << "  max_time: " << config.max_time << " s" << std::endl;
        std::cout << "  UDP port: " << config.udp_port << std::endl;
        std::cout << "  Telemetry rate: " << config.telemetry_rate_hz << " Hz" << std::endl;
        std::cout << "  P_max: " << config.P_max << " Pa" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        // Main simulation loop
        auto last_telemetry = std::chrono::steady_clock::now();
        double telemetry_interval = 1.0 / config.telemetry_rate_hz;
        
        while (g_running && !simulator.isFinished()) {
            auto loop_start = std::chrono::steady_clock::now();
            
            // Process incoming commands
            while (udp_server.hasCommands()) {
                Command cmd = udp_server.getNextCommand();
                if (!cmd.valve_id.empty()) {
                    ValveState state = (cmd.state == "open") ? ValveState::OPEN : ValveState::CLOSED;
                    if (simulator.setValveState(cmd.valve_id, state)) {
                        std::cout << "Set " << cmd.valve_id << " to " << cmd.state << std::endl;
                    } else {
                        std::cout << "Failed to set " << cmd.valve_id << " to " << cmd.state << std::endl;
                    }
                }
            }
            
            // Step simulation
            simulator.step();
            
            // Check for safety violations and alerts
            if (simulator.checkSafetyViolation()) {
                Alert alert;
                alert.name = "PRESSURE_SAFETY_VIOLATION";
                alert.value = simulator.getState().p2;
                alert.timestamp = std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                alert.description = "Cylinder pressure exceeded safety limit";
                udp_server.sendAlert(alert);
            }
            
            if (!simulator.checkSimulationStability()) {
                Alert alert;
                alert.name = "SIMULATION_INSTABILITY_DETECTED";
                alert.value = 0.0;
                alert.timestamp = std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                alert.description = "Simulation values became unstable (NaN/inf)";
                udp_server.sendAlert(alert);
                break;
            }
            
            if (!simulator.checkPhysicalValidity()) {
                Alert alert;
                alert.name = "NEGATIVE_PRESSURE_ERROR";
                alert.value = std::min(simulator.getState().p1, simulator.getState().p2);
                alert.timestamp = std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                alert.description = "Negative pressure detected";
                udp_server.sendAlert(alert);
            }
            
            // Send telemetry at specified rate
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<double>(now - last_telemetry).count() >= telemetry_interval) {
                udp_server.sendTelemetry(simulator.getState());
                last_telemetry = now;
            }
            
            // Sleep to maintain real-time execution
            auto loop_end = std::chrono::steady_clock::now();
            auto loop_duration = std::chrono::duration<double>(loop_end - loop_start).count();
            double sleep_time = config.dt - loop_duration;
            
            if (sleep_time > 0) {
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
            }
        }
        
        std::cout << "Simulation completed." << std::endl;
        udp_server.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
