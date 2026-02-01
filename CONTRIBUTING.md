# Contributing to Hydraulics Simulation System

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Development Setup

### Prerequisites

- CMake 3.16+
- C++17 compiler (GCC 11+ or Clang 14+)
- Docker & Docker Compose (for SIL testing)
- clang-format (for code formatting)
- cppcheck (for static analysis)

### Building for Development

```bash
cd cpp_simulator

# Debug build with coverage
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build-debug -j$(nproc)

# Run tests
cd build-debug && ./simulator_tests
```

## Code Style

We follow a style based on the Google C++ Style Guide with some embedded-friendly modifications.

### Formatting

Before committing, format your code:

```bash
# Format all files
find cpp_simulator -name '*.cpp' -o -name '*.h' | xargs clang-format -i

# Check formatting without modifying
find cpp_simulator -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror
```

### Naming Conventions

| Item | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `FillValve` |
| Functions | camelCase | `calculateFlow()` |
| Member variables | trailing underscore | `pressure_` |
| Constants | UPPER_SNAKE_CASE | `MAX_PRESSURE` |
| Local variables | snake_case | `flow_rate` |

### Documentation

- Use Doxygen-style comments for public APIs
- Add implementation comments for non-obvious logic
- Update README.md if adding new features

```cpp
/**
 * @brief Calculate flow through the valve
 * @param p1 Upstream pressure (Pa)
 * @param p2 Downstream pressure (Pa)
 * @return Flow rate (m³/s), positive = forward flow
 */
double calculateFlow(double p1, double p2) const;
```

## Testing

### Unit Tests

All new features must include unit tests:

```cpp
TEST(FeatureTest, BehaviorUnderCondition) {
    // Arrange
    FillValve valve(1e-5);
    
    // Act
    valve.open();
    double flow = valve.calculateFlow(1000.0, 0.0);
    
    // Assert
    EXPECT_GT(flow, 0.0);
}
```

### Integration Tests

For larger features, add integration tests in the `python_client/tests/` directory.

### Running the Full Test Suite

```bash
# Quick unit tests
cd cpp_simulator/build && ./simulator_tests

# Full SIL test suite
./run_tests.sh --all-tests

# Memory safety checks
./run_tests.sh --with-asan
```

## Pull Request Process

1. **Fork** the repository and create a feature branch
2. **Write tests** for your changes
3. **Ensure all tests pass** locally
4. **Format your code** with clang-format
5. **Update documentation** if needed
6. **Create a PR** with a clear description

### PR Checklist

- [ ] Code follows the style guide
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated
- [ ] No new warnings from cppcheck
- [ ] Commit messages are clear and descriptive

### Commit Messages

Use conventional commit format:

```text
type(scope): description

[optional body]

[optional footer]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
- `feat(valve): add proportional valve support`
- `fix(simulator): prevent negative pressure values`
- `docs(readme): update build instructions`
- `test(volume): add edge case tests`

## Architecture Guidelines

### Adding New Components

1. Create header in `include/` with clear interface
2. Implement in `src/` with full documentation
3. Add unit tests in `tests/`
4. Update CMakeLists.txt if needed
5. Document in README.md

### Thread Safety

The simulator runs in a single thread with UDP I/O on separate threads. When modifying:
- State mutations must be in the main simulation thread
- UDP server handles its own thread safety
- Use atomic types for cross-thread flags

### Performance Considerations

- This is a real-time simulator; avoid allocations in the main loop
- Use move semantics where appropriate
- Profile before optimizing

## Questions?

Open an issue for questions about contributing. We're happy to help!
