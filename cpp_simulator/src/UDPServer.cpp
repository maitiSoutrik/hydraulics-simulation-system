#include "UDPServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <cstring>

UDPServer::UDPServer(int port, double telemetry_rate_hz)
    : socket_fd_(-1), port_(port), running_(false), telemetry_rate_hz_(telemetry_rate_hz) {
}

UDPServer::~UDPServer() {
    stop();
}

bool UDPServer::start() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(socket_fd_);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(socket_fd_);
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&UDPServer::serverLoop, this);
    
    std::cout << "UDP Server started on port " << port_ << std::endl;
    return true;
}

void UDPServer::stop() {
    if (running_) {
        running_ = false;
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
        
        std::cout << "UDP Server stopped" << std::endl;
    }
}

bool UDPServer::hasCommands() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    return !command_queue_.empty();
}

Command UDPServer::getNextCommand() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (command_queue_.empty()) {
        return {"", "", "", 0.0, 0};
    }
    
    Command cmd = command_queue_.front();
    command_queue_.pop();
    return cmd;
}

void UDPServer::sendAlert(const Alert& alert) {
    nlohmann::json alert_msg = createAlertMessage(alert);
    sendMessageToAllClients(alert_msg);
}

void UDPServer::sendTelemetry(const SimulationState& state) {
    nlohmann::json telemetry_msg = createTelemetryMessage(state);
    sendMessageToAllClients(telemetry_msg);
}

void UDPServer::serverLoop() {
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (running_) {
        // Set timeout for recvfrom
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socket_fd_, &readfds);
        
        int result = select(socket_fd_ + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (result > 0 && FD_ISSET(socket_fd_, &readfds)) {
            ssize_t bytes_received = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                                            (struct sockaddr*)&client_addr, &client_len);
            
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::string message(buffer);
                processIncomingMessage(message, client_addr);
            }
        }
    }
}

void UDPServer::processIncomingMessage(const std::string& message, 
                                     const struct sockaddr_in& client_addr) {
    try {
        nlohmann::json json_msg = nlohmann::json::parse(message);
        
        if (json_msg.contains("command")) {
            std::string command_type = json_msg["command"];
            
            if (command_type == "set_valve") {
                Command cmd;
                cmd.command = command_type;
                cmd.valve_id = json_msg["valve_id"];
                cmd.state = json_msg["state"];
                cmd.timestamp = std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                
                std::lock_guard<std::mutex> lock(command_mutex_);
                command_queue_.push(cmd);

            } else if (command_type == "register_client") {
                struct sockaddr_in new_client_addr = client_addr;
                new_client_addr.sin_port = htons(json_msg["port"]);

                std::lock_guard<std::mutex> lock(client_mutex_);
                // Avoid duplicate entries
                bool found = false;
                for (const auto& addr : client_addresses_) {
                    if (addr.sin_addr.s_addr == new_client_addr.sin_addr.s_addr &&
                        addr.sin_port == new_client_addr.sin_port) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    client_addresses_.push_back(new_client_addr);
                    std::cout << "Registered new client from port " << json_msg["port"] << std::endl;
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        
        // Send invalid command alert
        Alert alert;
        alert.name = "INVALID_COMMAND_RECEIVED";
        alert.value = 0.0;
        alert.timestamp = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        alert.description = "Malformed JSON received: " + std::string(e.what());
        
        sendAlert(alert);
    }
}

nlohmann::json UDPServer::createTelemetryMessage(const SimulationState& state) {
    nlohmann::json msg;
    msg["type"] = "telemetry";
    msg["time"] = state.time;
    msg["p_cyl"] = state.p2;
    msg["p_acc"] = state.p1;
    
    // Add valve states
    for (const auto& valve : state.valve_states) {
        std::string valve_state = (valve.second == ValveState::OPEN) ? "open" : "closed";
        msg["valves"][valve.first] = valve_state;
    }
    
    return msg;
}

nlohmann::json UDPServer::createAlertMessage(const Alert& alert) {
    nlohmann::json msg;
    msg["type"] = "alert";
    msg["name"] = alert.name;
    msg["value"] = alert.value;
    msg["timestamp"] = alert.timestamp;
    msg["description"] = alert.description;
    
    return msg;
}

bool UDPServer::sendMessageToAllClients(const nlohmann::json& message) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    bool all_sent = true;
    for (const auto& client_addr : client_addresses_) {
        if (!sendMessage(message, client_addr)) {
            all_sent = false;
        }
    }
    return all_sent;
}

bool UDPServer::sendMessage(const nlohmann::json& message, 
                          const struct sockaddr_in& client_addr) {
    std::string msg_str = message.dump();
    
    ssize_t bytes_sent = sendto(socket_fd_, msg_str.c_str(), msg_str.length(), 0,
                               (struct sockaddr*)&client_addr, sizeof(client_addr));
    
    return bytes_sent > 0;
}
