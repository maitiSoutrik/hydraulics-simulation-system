import socket
import json
import time
import threading
from typing import Optional, Dict, Any
from dataclasses import dataclass

@dataclass
class TelemetryData:
    time: float
    p_cyl: float
    p_acc: float
    valves: Dict[str, str]

@dataclass
class AlertData:
    name: str
    value: float
    timestamp: float
    description: str

class HydraulicsClient:
    def __init__(self, server_host: str = "localhost", server_port: int = 8080):
        self.server_host = server_host
        self.server_port = server_port
        self.listen_port = server_port + 1  # Listen on server_port + 1
        
        self.command_socket = None
        self.listen_socket = None
        self.listening = False
        self.listen_thread = None
        
        self.latest_telemetry: Optional[TelemetryData] = None
        self.latest_alert: Optional[AlertData] = None
        self.telemetry_history = []
        self.alert_history = []
        
        self._telemetry_lock = threading.Lock()
        self._alert_lock = threading.Lock()
    
    def connect(self) -> bool:
        """Connect to the hydraulics simulator and register for telemetry"""
        try:
            # Create command socket
            self.command_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            
            # Create listening socket
            self.listen_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.listen_socket.bind(("", self.listen_port))
            self.listen_socket.settimeout(0.1)  # 100ms timeout
            
            # Start listening thread
            self.listening = True
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Register with the simulator to receive telemetry
            if not self.register_client():
                raise ConnectionError("Failed to register with simulator")
            
            print(f"Connected to hydraulics simulator at {self.server_host}:{self.server_port}")
            return True
            
        except Exception as e:
            print(f"Failed to connect: {e}")
            self.disconnect()
            return False
    
    def disconnect(self):
        """Disconnect from the simulator"""
        self.listening = False
        
        if self.listen_thread and self.listen_thread.is_alive():
            self.listen_thread.join(timeout=1.0)
        
        if self.command_socket:
            self.command_socket.close()
            self.command_socket = None
            
        if self.listen_socket:
            self.listen_socket.close()
            self.listen_socket = None
        
        print("Disconnected from hydraulics simulator")
    
    def send_command(self, command_data: Dict[str, Any]) -> bool:
        """Send a generic command to the simulator"""
        if not self.command_socket:
            print("Not connected to simulator")
            return False
        
        try:
            message = json.dumps(command_data).encode('utf-8')
            self.command_socket.sendto(message, (self.server_host, self.server_port))
            
            cmd_type = command_data.get('command', 'unknown')
            print(f"Sent command: {cmd_type}")
            return True
            
        except Exception as e:
            print(f"Failed to send command: {e}")
            return False

    def set_valve_state(self, valve_id: str, state: str) -> bool:
        """Convenience method to send a valve command"""
        command = {
            "command": "set_valve",
            "valve_id": valve_id,
            "state": state
        }
        return self.send_command(command)

    def register_client(self) -> bool:
        """Register this client with the simulator to receive telemetry"""
        command = {
            "command": "register_client",
            "port": self.listen_port
        }
        return self.send_command(command)
    
    def get_latest_telemetry(self) -> Optional[TelemetryData]:
        """Get the most recent telemetry data"""
        with self._telemetry_lock:
            return self.latest_telemetry
    
    def get_latest_alert(self) -> Optional[AlertData]:
        """Get the most recent alert"""
        with self._alert_lock:
            return self.latest_alert
    
    def get_telemetry_history(self) -> list:
        """Get all telemetry data received"""
        with self._telemetry_lock:
            return self.telemetry_history.copy()
    
    def get_alert_history(self) -> list:
        """Get all alerts received"""
        with self._alert_lock:
            return self.alert_history.copy()
    
    def wait_for_telemetry(self, timeout: float = 5.0) -> Optional[TelemetryData]:
        """Wait for new telemetry data"""
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            with self._telemetry_lock:
                if self.latest_telemetry is not None:
                    return self.latest_telemetry
            time.sleep(0.1)
        
        return None
    
    def _listen_loop(self):
        """Background thread to listen for telemetry and alerts"""
        while self.listening:
            try:
                data, addr = self.listen_socket.recvfrom(1024)
                message = json.loads(data.decode('utf-8'))
                
                if message.get("type") == "telemetry":
                    telemetry = TelemetryData(
                        time=message["time"],
                        p_cyl=message["p_cyl"],
                        p_acc=message["p_acc"],
                        valves=message.get("valves", {})
                    )
                    
                    with self._telemetry_lock:
                        self.latest_telemetry = telemetry
                        self.telemetry_history.append(telemetry)
                        
                        # Keep only last 1000 entries
                        if len(self.telemetry_history) > 1000:
                            self.telemetry_history = self.telemetry_history[-1000:]
                
                elif message.get("type") == "alert":
                    alert = AlertData(
                        name=message["name"],
                        value=message["value"],
                        timestamp=message["timestamp"],
                        description=message["description"]
                    )
                    
                    with self._alert_lock:
                        self.latest_alert = alert
                        self.alert_history.append(alert)
                        
                        # Keep only last 100 alerts
                        if len(self.alert_history) > 100:
                            self.alert_history = self.alert_history[-100:]
                    
                    print(f"ALERT: {alert.name} - {alert.description}")
                
            except socket.timeout:
                continue
            except json.JSONDecodeError as e:
                print(f"JSON decode error: {e}")
            except Exception as e:
                if self.listening:  # Only print if we're still supposed to be listening
                    print(f"Listen loop error: {e}")
    
    def __enter__(self):
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
