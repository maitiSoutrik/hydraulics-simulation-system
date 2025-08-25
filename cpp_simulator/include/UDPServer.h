#pragma once
#include "Simulator.h"
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>
#include <netinet/in.h>

struct Command {
    std::string command;
    std::string valve_id;
    std::string state;
    double timestamp;
    int port;
};

struct Alert {
    std::string name;
    double value;
    double timestamp;
    std::string description;
};

class UDPServer {
private:
    int socket_fd_;
    int port_;
    std::atomic<bool> running_;
    
    // Threading
    std::thread server_thread_;
    std::thread telemetry_thread_;
    
    // Command queue (thread-safe)
    std::queue<Command> command_queue_;
    std::mutex command_mutex_;
    
    // Alert queue (thread-safe)
    std::queue<Alert> alert_queue_;
    std::mutex alert_mutex_;
    
    // Client list (thread-safe)
    std::vector<struct sockaddr_in> client_addresses_;
    std::mutex client_mutex_;

    // Telemetry settings
    double telemetry_rate_hz_;
    
public:
    UDPServer(int port, double telemetry_rate_hz);
    ~UDPServer();
    
    // Server control
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Command handling
    bool hasCommands();
    Command getNextCommand();
    
    // Alert handling
    void sendAlert(const Alert& alert);
    
    // Telemetry
    void sendTelemetry(const SimulationState& state);
    
private:
    void serverLoop();
    void telemetryLoop(Simulator* simulator);
    void processIncomingMessage(const std::string& message, 
                               const struct sockaddr_in& client_addr);
    
    nlohmann::json createTelemetryMessage(const SimulationState& state);
    nlohmann::json createAlertMessage(const Alert& alert);
    
    bool sendMessage(const nlohmann::json& message, 
                    const struct sockaddr_in& client_addr);
    bool sendMessageToAllClients(const nlohmann::json& message);
};
