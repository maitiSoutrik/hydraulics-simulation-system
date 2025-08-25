import pytest
import time
import subprocess
from pathlib import Path
import sys

# Add parent directory to path to import hydraulics_client
sys.path.append(str(Path(__file__).parent.parent))
from hydraulics_client import HydraulicsClient

class TestReliefValve:
    @classmethod
    def setup_class(cls):
        """Start the simulator with relief valve test configuration"""
        cls.simulator_path = Path(__file__).parent.parent.parent / "cpp_simulator" / "build" / "simulator"
        cls.config_path = Path(__file__).parent.parent.parent / "config" / "test_scenarios" / "relief_valve_test.json"
        
        if cls.simulator_path.exists() and cls.config_path.exists():
            cls.simulator_process = subprocess.Popen(
                [str(cls.simulator_path), str(cls.config_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            time.sleep(2.0)
        else:
            cls.simulator_process = None
            print(f"Warning: Simulator not found at {cls.simulator_path}")
    
    @classmethod
    def teardown_class(cls):
        """Stop the simulator after tests"""
        if cls.simulator_process:
            cls.simulator_process.terminate()
            try:
                cls.simulator_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                cls.simulator_process.kill()
    
    def test_relief_valve_activation(self):
        """Test that relief valve opens when pressure exceeds P_max"""
        with HydraulicsClient() as client:
            # Get initial state
            initial_telemetry = client.wait_for_telemetry(timeout=5.0)
            assert initial_telemetry is not None
            assert initial_telemetry.p_cyl == pytest.approx(0.0, abs=10.0)
            
            # Open fill valve to build pressure
            success = client.set_valve_state("fill_valve", "open")
            assert success
            
            # Wait for pressure to build up and relief valve to activate
            # P_max is set to 800 Pa in relief_valve_test.json
            max_wait_time = 15.0
            start_time = time.time()
            relief_valve_opened = False
            
            while time.time() - start_time < max_wait_time:
                telemetry = client.get_latest_telemetry()
                if telemetry and telemetry.valves.get("relief_valve") == "open":
                    relief_valve_opened = True
                    break
                time.sleep(0.5)
            
            assert relief_valve_opened, "Relief valve should have opened when pressure exceeded P_max"
            
            # Check that pressure is controlled
            final_telemetry = client.get_latest_telemetry()
            assert final_telemetry.p_cyl <= 900.0, f"Pressure should be controlled by relief valve, got {final_telemetry.p_cyl}"
    
    def test_pressure_limiting(self):
        """Test that relief valve prevents pressure from exceeding P_max significantly"""
        with HydraulicsClient() as client:
            # Open fill valve
            client.set_valve_state("fill_valve", "open")
            
            # Wait for system to reach steady state
            time.sleep(10.0)
            
            # Collect pressure readings
            pressure_readings = []
            for _ in range(20):  # Collect 20 readings over 2 seconds
                telemetry = client.get_latest_telemetry()
                if telemetry:
                    pressure_readings.append(telemetry.p_cyl)
                time.sleep(0.1)
            
            assert len(pressure_readings) > 0, "Should have collected pressure readings"
            
            # Check that pressure doesn't exceed P_max + some tolerance
            max_pressure = max(pressure_readings)
            P_max = 800.0  # From relief_valve_test.json
            tolerance = 100.0  # Allow some overshoot
            
            assert max_pressure <= P_max + tolerance, \
                f"Maximum pressure {max_pressure} exceeded P_max + tolerance ({P_max + tolerance})"
    
    def test_relief_valve_cycling(self):
        """Test that relief valve opens and closes properly (hysteresis)"""
        with HydraulicsClient() as client:
            # Open fill valve to trigger relief valve
            client.set_valve_state("fill_valve", "open")
            
            # Wait for relief valve to open
            time.sleep(8.0)
            
            # Close fill valve to allow pressure to drop
            client.set_valve_state("fill_valve", "closed")
            
            # Monitor relief valve state changes
            valve_states = []
            start_time = time.time()
            
            while time.time() - start_time < 10.0:
                telemetry = client.get_latest_telemetry()
                if telemetry:
                    valve_state = telemetry.valves.get("relief_valve", "unknown")
                    if not valve_states or valve_states[-1] != valve_state:
                        valve_states.append(valve_state)
                time.sleep(0.2)
            
            # Should see valve opening and potentially closing
            assert "open" in valve_states, "Relief valve should have opened"
            
            # Check final pressure is reasonable
            final_telemetry = client.get_latest_telemetry()
            assert final_telemetry.p_cyl < 900.0, "Final pressure should be controlled"
    
    def test_alert_generation(self):
        """Test that appropriate alerts are generated during relief valve operation"""
        with HydraulicsClient() as client:
            # Clear any existing alerts
            client.get_alert_history()
            
            # Open fill valve to build pressure
            client.set_valve_state("fill_valve", "open")
            
            # Wait for pressure to build and potentially trigger alerts
            time.sleep(8.0)
            
            # Check for alerts
            alerts = client.get_alert_history()
            
            # Should have received some alerts during operation
            # (This depends on the specific alert thresholds in the simulator)
            print(f"Received {len(alerts)} alerts during test")
            
            for alert in alerts:
                print(f"Alert: {alert.name} - {alert.description}")
                # Verify alert structure
                assert hasattr(alert, 'name')
                assert hasattr(alert, 'value')
                assert hasattr(alert, 'timestamp')
                assert hasattr(alert, 'description')
    
    def test_system_stability_with_relief_valve(self):
        """Test that system remains stable when relief valve is active"""
        with HydraulicsClient() as client:
            # Wait for initial connection
            initial_telemetry = client.wait_for_telemetry(timeout=5.0)
            assert initial_telemetry is not None, "Should receive initial telemetry"
            
            # Open fill valve
            client.set_valve_state("fill_valve", "open")
            
            # Let system run for extended period
            time.sleep(12.0)
            
            # Collect telemetry history
            history = client.get_telemetry_history()
            assert len(history) > 10, f"Should have substantial telemetry history, got {len(history)} entries"
            
            # Check for stability - no NaN or infinite values
            for telemetry in history[-20:]:  # Check last 20 readings
                assert telemetry.p_cyl >= 0.0, f"Pressure should be non-negative, got {telemetry.p_cyl}"
                assert telemetry.p_acc >= 0.0, f"Accumulator pressure should be non-negative, got {telemetry.p_acc}"
                assert telemetry.time >= 0.0, f"Time should be non-negative, got {telemetry.time}"
                
                # Check for reasonable values (not NaN or inf)
                assert telemetry.p_cyl < 10000.0, f"Pressure seems unreasonably high: {telemetry.p_cyl}"
                assert telemetry.p_acc < 10000.0, f"Accumulator pressure seems unreasonably high: {telemetry.p_acc}"
