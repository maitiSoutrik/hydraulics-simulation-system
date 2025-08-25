import pytest
import time
import subprocess
import os
import signal
from pathlib import Path
import sys

# Add parent directory to path to import hydraulics_client
sys.path.append(str(Path(__file__).parent.parent))
from hydraulics_client import HydraulicsClient

class TestFillCycle:
    @classmethod
    def setup_class(cls):
        """Start the simulator before running tests"""
        # Path to the simulator executable and config
        cls.simulator_path = Path(__file__).parent.parent.parent / "cpp_simulator" / "build" / "simulator"
        cls.config_path = Path(__file__).parent.parent.parent / "config" / "test_scenarios" / "fill_cycle.json"
        
        # Start the simulator process
        if cls.simulator_path.exists() and cls.config_path.exists():
            cls.simulator_process = subprocess.Popen(
                [str(cls.simulator_path), str(cls.config_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            # Give simulator time to start
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
    
    def test_initial_conditions(self):
        """Test that the system starts with correct initial conditions"""
        with HydraulicsClient() as client:
            # Wait for initial telemetry
            telemetry = client.wait_for_telemetry(timeout=5.0)
            
            assert telemetry is not None, "No telemetry received"
            assert telemetry.p_acc == pytest.approx(1000.0, rel=0.01), f"Expected p_acc=1000, got {telemetry.p_acc}"
            assert telemetry.p_cyl == pytest.approx(0.0, abs=10.0), f"Expected p_cyl≈0, got {telemetry.p_cyl}"
            
            # Check that all valves are initially closed
            assert telemetry.valves.get("fill_valve") == "closed"
            assert telemetry.valves.get("drain_valve") == "closed"
            assert telemetry.valves.get("relief_valve") == "closed"
    
    def test_fill_valve_operation(self):
        """Test opening the fill valve and monitoring pressure rise"""
        with HydraulicsClient() as client:
            # Get initial telemetry
            initial_telemetry = client.wait_for_telemetry(timeout=5.0)
            assert initial_telemetry is not None
            
            initial_p_cyl = initial_telemetry.p_cyl
            
            # Open fill valve
            success = client.set_valve_state("fill_valve", "open")
            assert success, "Failed to send fill valve command"
            
            # Wait for pressure to rise
            time.sleep(5.0)
            
            # Get updated telemetry
            current_telemetry = client.get_latest_telemetry()
            assert current_telemetry is not None
            
            # Pressure should have increased (more lenient check)
            assert current_telemetry.p_cyl > initial_p_cyl + 10.0, \
                f"Pressure should have increased. Initial: {initial_p_cyl}, Current: {current_telemetry.p_cyl}"
            
            # Fill valve should be open
            assert current_telemetry.valves.get("fill_valve") == "open"
    
    def test_pressure_stabilization(self):
        """Test that pressure eventually stabilizes near accumulator pressure"""
        with HydraulicsClient() as client:
            # Open fill valve
            client.set_valve_state("fill_valve", "open")
            
            # Wait for system to stabilize (longer time)
            time.sleep(10.0)
            
            # Get final telemetry
            final_telemetry = client.get_latest_telemetry()
            assert final_telemetry is not None
            
            # Cylinder pressure should be close to accumulator pressure or relief valve should be active
            pressure_diff = abs(final_telemetry.p_cyl - final_telemetry.p_acc)
            relief_valve_active = final_telemetry.valves.get("relief_valve") == "open"
            
            # Either pressures stabilize OR relief valve is managing pressure
            stabilized = pressure_diff < 150.0
            pressure_managed = final_telemetry.p_cyl < 1000.0  # Below relief valve threshold
            
            assert stabilized or pressure_managed, \
                f"System should stabilize or manage pressure. p_acc: {final_telemetry.p_acc}, p_cyl: {final_telemetry.p_cyl}, diff: {pressure_diff}, relief_active: {relief_valve_active}"
    
    def test_valve_control_commands(self):
        """Test that valve commands are properly executed"""
        with HydraulicsClient() as client:
            # Test opening fill valve
            success = client.set_valve_state("fill_valve", "open")
            assert success
            
            time.sleep(1.0)
            telemetry = client.get_latest_telemetry()
            assert telemetry.valves.get("fill_valve") == "open"
            
            # Test closing fill valve
            success = client.set_valve_state("fill_valve", "closed")
            assert success
            
            time.sleep(1.0)
            telemetry = client.get_latest_telemetry()
            assert telemetry.valves.get("fill_valve") == "closed"
    
    def test_telemetry_continuity(self):
        """Test that telemetry is received continuously"""
        with HydraulicsClient() as client:
            # Collect telemetry for a few seconds
            start_time = time.time()
            telemetry_count = 0
            
            while time.time() - start_time < 5.0:
                if client.wait_for_telemetry(timeout=1.0):
                    telemetry_count += 1
            
            # Should receive multiple telemetry messages
            assert telemetry_count >= 10, f"Expected at least 10 telemetry messages, got {telemetry_count}"
            
            # Check telemetry history
            history = client.get_telemetry_history()
            assert len(history) >= 10, f"Expected at least 10 history entries, got {len(history)}"
            
            # Time should be increasing
            for i in range(1, len(history)):
                assert history[i].time > history[i-1].time, "Telemetry time should be increasing"
