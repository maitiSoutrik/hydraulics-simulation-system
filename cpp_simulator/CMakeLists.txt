cmake_minimum_required(VERSION 3.16)
project(HydraulicsSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Add nlohmann/json (header-only library)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Add GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Source files
set(SOURCES
    src/Volume.cpp
    src/Orifice.cpp
    src/FillValve.cpp
    src/ReliefValve.cpp
    src/Simulator.cpp
    src/ConfigLoader.cpp
    src/UDPServer.cpp
)

# Create the main executable
add_executable(simulator
    ${SOURCES}
    src/main.cpp
)

target_link_libraries(simulator 
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Create test executable
add_executable(simulator_tests
    ${SOURCES}
    tests/test_volume.cpp
    tests/test_fill_valve.cpp
    tests/test_relief_valve.cpp
    tests/test_simulator.cpp
    tests/test_error_handling.cpp
)

target_link_libraries(simulator_tests
    nlohmann_json::nlohmann_json
    gtest_main
    Threads::Threads
)

# Enable testing
enable_testing()
add_test(NAME simulator_unit_tests COMMAND simulator_tests)
