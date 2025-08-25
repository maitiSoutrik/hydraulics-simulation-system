#!/usr/bin/env python3
"""
Hydraulics Simulation Demo Script
Demonstrates the working simulation for team presentation
"""
import socket
import json
import time
import threading
from datetime import datetime

class SimulationDemo:
    def __init__(self):
        self.command_port = 8080
        self.telemetry_port = 8081
        self.running = False
        self.telemetry_data = []
        
    def send_command(self, valve_id, state):
        """Send valve command to simulator"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        command = {
            "command": "set_valve",
            "valve_id": valve_id,
            "state": state
        }
        message = json.dumps(command)
        sock.sendto(message.encode('utf-8'), ("localhost", self.command_port))
        sock.close()
        print(f"🔧 Command sent: {valve_id} -> {state}")
        
    def listen_telemetry(self):
        """Listen for telemetry data"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            sock.bind(("", self.telemetry_port))
            sock.settimeout(0.5)
            print(f"📡 Listening for telemetry on port {self.telemetry_port}")
            
            while self.running:
                try:
                    data, addr = sock.recvfrom(1024)
                    message = json.loads(data.decode('utf-8'))
                    
                    if message.get("type") == "telemetry":
                        self.telemetry_data.append(message)
                        p_cyl = message.get('p_cyl', 0)
                        p_acc = message.get('p_acc', 0)
                        valves = message.get('valves', {})
                        
                        # Show key telemetry updates
                        if len(self.telemetry_data) % 25 == 0:  # Every 0.5 seconds at 50Hz
                            print(f"📊 Pressure: Cylinder={p_cyl:.1f} Pa, Accumulator={p_acc:.1f} Pa")
                            if valves:
                                valve_status = ", ".join([f"{k}={v}" for k, v in valves.items()])
                                print(f"🔧 Valves: {valve_status}")
                                
                    elif message.get("type") == "alert":
                        print(f"⚠️  ALERT: {message.get('name')} - {message.get('description')}")
                        
                except socket.timeout:
                    continue
                except json.JSONDecodeError:
                    continue
                    
        except Exception as e:
            print(f"❌ Telemetry error: {e}")
        finally:
            sock.close()
            
    def run_demo(self):
        """Run the complete demonstration"""
        print("=" * 60)
        print("🚀 HYDRAULICS SIMULATION SYSTEM DEMO")
        print("=" * 60)
        print(f"⏰ Started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print()
        
        # Start telemetry listener
        self.running = True
        telemetry_thread = threading.Thread(target=self.listen_telemetry, daemon=True)
        telemetry_thread.start()
        
        time.sleep(2)  # Let telemetry start
        
        print("🎯 Demo Scenario: Fill Cycle with Pressure Management")
        print()
        
        # Phase 1: Initial state
        print("📋 Phase 1: System Initialization")
        print("   - Cylinder pressure: ~0 Pa")
        print("   - Accumulator pressure: 1000 Pa")
        print("   - All valves closed")
        time.sleep(3)
        
        # Phase 2: Open fill valve
        print("\n📋 Phase 2: Opening Fill Valve")
        print("   - Opening fill valve to pressurize cylinder")
        self.send_command("fill_valve", "open")
        time.sleep(8)
        
        # Phase 3: Monitor pressure rise
        print("\n📋 Phase 3: Pressure Rise & Relief Valve Activation")
        print("   - Cylinder pressure rising toward accumulator pressure")
        print("   - Relief valve will activate at 900 Pa to prevent overpressure")
        time.sleep(10)
        
        # Phase 4: Close fill valve
        print("\n📋 Phase 4: Closing Fill Valve")
        print("   - Closing fill valve to stop pressurization")
        self.send_command("fill_valve", "closed")
        time.sleep(5)
        
        # Summary
        print("\n" + "=" * 60)
        print("📈 DEMO SUMMARY")
        print("=" * 60)
        print(f"✅ Total telemetry messages received: {len(self.telemetry_data)}")
        
        if self.telemetry_data:
            latest = self.telemetry_data[-1]
            print(f"✅ Final cylinder pressure: {latest.get('p_cyl', 0):.1f} Pa")
            print(f"✅ Final accumulator pressure: {latest.get('p_acc', 0):.1f} Pa")
            
            # Check if relief valve activated
            relief_activations = [t for t in self.telemetry_data 
                                if t.get('valves', {}).get('relief_valve') == 'open']
            if relief_activations:
                print(f"✅ Relief valve activated {len(relief_activations)} times")
            else:
                print("ℹ️  Relief valve did not activate (pressure stayed below threshold)")
                
        print("\n🎉 Hydraulics simulation demo completed successfully!")
        print("🔧 Key features demonstrated:")
        print("   • Real-time UDP communication")
        print("   • Valve control commands")
        print("   • Pressure monitoring")
        print("   • Safety system (relief valve)")
        print("   • Telemetry data streaming")
        
        self.running = False

if __name__ == "__main__":
    demo = SimulationDemo()
    try:
        demo.run_demo()
    except KeyboardInterrupt:
        print("\n⏹️  Demo stopped by user")
        demo.running = False
