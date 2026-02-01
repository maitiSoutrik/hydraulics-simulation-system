# Hydraulics Simulation System

[![CI/CD Pipeline](https://github.com/maitiSoutrik/hydraulics-simulation-system/actions/workflows/ci.yml/badge.svg)](https://github.com/maitiSoutrik/hydraulics-simulation-system/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A real-time hydraulics system simulator designed for **Software-in-the-Loop (SIL)** testing of embedded control systems. Built with modern C++17, this simulator models hydraulic circuits with accumulators, cylinders, fill valves, drain valves, and pressure relief valves.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                     Hydraulics Simulation System                     │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────┐     Fill Valve     ┌──────────────┐               │
│  │              │─────────────────►  │              │               │
│  │  Accumulator │                    │   Cylinder   │               │
│  │    (P1)      │◄─────────────────  │     (P2)     │               │
│  │              │     Drain Valve    │              │               │
│  └──────────────┘                    └──────┬───────┘               │
│        │                                    │                        │
│        │                              Relief Valve                   │
│        │                                    │                        │
│        │                                    ▼                        │
│        └──────────────────────────►  [ Tank (0 Pa) ]                │
│                                                                      │
├─────────────────────────────────────────────────────────────────────┤
│  UDP Server (Command/Telemetry)  ←→  Python Test Client             │
└─────────────────────────────────────────────────────────────────────┘
```

## Features

- **Real-time Simulation**: Configurable timestep with real-time execution
- **Modular Component Design**: Easily extendable valve and volume models
- **UDP Interface**: Send commands and receive telemetry via JSON over UDP
- **Safety Monitoring**: Automatic pressure limit checks and stability detection
- **Dockerized Testing**: Complete SIL test harness with visualization
- **CI/CD Pipeline**: Automated testing, coverage, and static analysis

## System Components

| Component | Description | Physics Model |
|-----------|-------------|---------------|
| **Volume** | Compressible fluid volume | `dP/dt = β/V × Q` (bulk modulus equation) |
| **FillValve** | On/off orifice valve | `Q = Cd × A × √(2×ΔP/ρ)` |
| **ReliefValve** | Pressure-actuated safety valve | Opens at P_max, closes at P_max - deadband |
| **Orifice** | Base flow restriction class | Turbulent orifice equation |

## Quick Start

### Prerequisites

- CMake 3.16+
- C++17 compiler (GCC 11+ or Clang 14+)
- Docker & Docker Compose (for SIL testing)

### Build from Source

```bash
cd cpp_simulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run Unit Tests

```bash
cd cpp_simulator/build
./simulator_tests
```

### Run SIL Test Suite

```bash
# Full test suite (fill cycle + relief valve tests)
./run_tests.sh --all-tests

# With visualization (requires X11)
./run_tests.sh --with-viz

# Just relief valve tests
./run_tests.sh --relief-test
```

## Configuration

The simulator is configured via JSON files. See `config/` for examples:

```json
{
  "dt": 0.001,
  "max_time": 60.0,
  "safety_pressure_limit": 1500.0,
  "p1_initial": 1000.0,
  "p2_initial": 0.0,
  "volume_cylinder": 0.001,
  "beta": 2.1e9,
  "P_max": 1200.0,
  "P_deadband": 50.0,
  "fill_valve_area": 1e-5,
  "drain_valve_area": 1e-5,
  "relief_valve_area": 1e-5,
  "udp_port": 5005,
  "telemetry_rate_hz": 100
}
```

## UDP Protocol

### Commands (to simulator)

```json
{"command": "set_valve", "valve_id": "fill_valve", "state": "open"}
{"command": "set_valve", "valve_id": "drain_valve", "state": "closed"}
```

### Telemetry (from simulator)

```json
{
  "type": "telemetry",
  "time": 1.234,
  "p1": 1000.0,
  "p2": 456.7,
  "valves": {
    "fill_valve": "open",
    "drain_valve": "closed",
    "relief_valve": "closed"
  }
}
```

### Alerts (from simulator)

```json
{
  "type": "alert",
  "name": "PRESSURE_SAFETY_VIOLATION",
  "value": 1523.4,
  "timestamp": 1706745600.123,
  "description": "Cylinder pressure exceeded safety limit"
}
```

## Project Structure

```
hydraulics-simulation-system/
├── cpp_simulator/
│   ├── include/           # Header files
│   │   ├── Simulator.h    # Main simulation engine
│   │   ├── Volume.h       # Compressible volume model
│   │   ├── FillValve.h    # On/off valve model
│   │   ├── ReliefValve.h  # Pressure relief valve
│   │   ├── Orifice.h      # Base orifice class
│   │   ├── UDPServer.h    # Network interface
│   │   └── ConfigLoader.h # JSON configuration
│   ├── src/               # Implementation files
│   ├── tests/             # Google Test unit tests
│   └── CMakeLists.txt     # Build configuration
├── python_client/         # Python test client
├── config/                # Configuration files
├── docker/                # Docker build files
├── docker-compose.yml     # SIL test orchestration
├── run_tests.sh           # Test runner script
└── README.md
```

## CI/CD Pipeline

The GitHub Actions workflow provides:

| Stage | Description |
|-------|-------------|
| **Static Analysis** | cppcheck, clang-format verification |
| **Build Matrix** | GCC and Clang compilation |
| **Unit Tests** | Google Test with coverage reporting |
| **Integration Tests** | Dockerized SIL test suite |
| **Memory Safety** | AddressSanitizer + Valgrind checks |
| **Documentation** | Doxygen API docs generation |

## Testing Strategy

### Unit Tests
- Component-level testing with Google Test
- Isolated valve and volume behavior verification
- Edge case and boundary condition coverage

### Integration Tests (SIL)
- Full system simulation with UDP communication
- Automated fill cycle and relief valve scenarios
- Timing and stability verification

### Safety Tests
- Pressure limit violation detection
- Simulation stability monitoring (NaN/inf checks)
- Physical validity constraints (no negative pressures)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Built for embedded systems interview preparation and SIL testing demonstrations
- Inspired by real hydraulic control system architectures
- Uses [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing
- Uses [Google Test](https://github.com/google/googletest) for unit testing
