#!/bin/bash

# SIL-Harness: Hydraulics Simulation Test Runner
# This script orchestrates the complete test suite using Docker Compose

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to cleanup containers
cleanup() {
    print_status "Cleaning up containers..."
    docker-compose down --remove-orphans 2>/dev/null || true
    docker-compose --profile relief-test down --remove-orphans 2>/dev/null || true
}

# Trap to ensure cleanup on exit
trap cleanup EXIT

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --build-only     Build Docker images only, don't run tests"
    echo "  --tests-only     Run tests only (assumes images are built)"
    echo "  --with-viz       Run with visualizer (requires X11 forwarding)"
    echo "  --relief-test    Run relief valve tests instead of fill cycle"
    echo "  --all-tests      Run both fill cycle and relief valve tests"
    echo "  --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run basic fill cycle tests"
    echo "  $0 --with-viz         # Run tests with visualization"
    echo "  $0 --all-tests        # Run complete test suite"
    echo "  $0 --build-only       # Just build the Docker images"
}

# Parse command line arguments
BUILD_ONLY=false
TESTS_ONLY=false
WITH_VIZ=false
RELIEF_TEST=false
ALL_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --tests-only)
            TESTS_ONLY=true
            shift
            ;;
        --with-viz)
            WITH_VIZ=true
            shift
            ;;
        --relief-test)
            RELIEF_TEST=true
            shift
            ;;
        --all-tests)
            ALL_TESTS=true
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Print banner
echo "=================================================================="
echo "  SIL-Harness: Hydraulics Simulation & Test Environment"
echo "  A containerized regression test suite with visualization"
echo "=================================================================="
echo ""

# Check if Docker and Docker Compose are available
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed or not in PATH"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    print_error "Docker Compose is not installed or not in PATH"
    exit 1
fi

# Create test results directory
mkdir -p test-results

# Build phase
if [[ "$TESTS_ONLY" != true ]]; then
    print_status "Building Docker images..."
    docker-compose build
    
    if [[ "$BUILD_ONLY" == true ]]; then
        print_success "Docker images built successfully!"
        exit 0
    fi
fi

# Test execution phase
TEST_EXIT_CODE=0

if [[ "$ALL_TESTS" == true ]]; then
    print_status "Running complete test suite (fill cycle + relief valve tests)..."
    
    # Run fill cycle tests
    print_status "Starting fill cycle test scenario..."
    if [[ "$WITH_VIZ" == true ]]; then
        print_warning "Visualizer requires X11 forwarding. Make sure DISPLAY is set."
        docker-compose up -d simulator visualizer
    else
        docker-compose up -d simulator
    fi
    
    # Wait for simulator to be ready
    print_status "Waiting for simulator to be ready..."
    sleep 5
    
    # Run fill cycle tests
    print_status "Running fill cycle tests..."
    docker-compose run --rm test-runner || TEST_EXIT_CODE=$?
    
    # Stop fill cycle services
    docker-compose down
    
    # Run relief valve tests
    print_status "Starting relief valve test scenario..."
    docker-compose --profile relief-test up -d simulator-relief
    
    # Wait for simulator to be ready
    sleep 5
    
    # Run relief valve tests
    print_status "Running relief valve tests..."
    docker-compose --profile relief-test run --rm test-runner-relief || TEST_EXIT_CODE=$?
    
    # Stop relief valve services
    docker-compose --profile relief-test down

elif [[ "$RELIEF_TEST" == true ]]; then
    print_status "Running relief valve test scenario..."
    
    # Start relief valve simulator
    docker-compose --profile relief-test up -d simulator-relief
    
    # Wait for simulator to be ready
    print_status "Waiting for simulator to be ready..."
    sleep 5
    
    # Run relief valve tests
    print_status "Running relief valve tests..."
    docker-compose --profile relief-test run --rm test-runner-relief || TEST_EXIT_CODE=$?

else
    print_status "Running fill cycle test scenario..."
    
    # Start services
    if [[ "$WITH_VIZ" == true ]]; then
        print_status "Starting simulator for visualization..."
        # Use custom docker-compose config for visualization
        docker-compose -f docker-compose.yml -f docker-compose.viz.yml up -d simulator
        
        print_status "Simulator started. To see visualization:"
        print_status "  Run in a separate terminal: ./run_visualizer.sh"
        
        # Wait a bit for simulator to start
        sleep 3
    else
        print_status "Starting simulator..."
        docker-compose up -d simulator
    fi
    
    # Wait for simulator to be ready
    print_status "Waiting for simulator to be ready..."
    sleep 5
    
    # Run tests
    print_status "Running fill cycle tests..."
    docker-compose run --rm test-runner || TEST_EXIT_CODE=$?
fi

# Generate final report
echo ""
echo "=================================================================="
echo "                        TEST RESULTS"
echo "=================================================================="

if [[ $TEST_EXIT_CODE -eq 0 ]]; then
    print_success "All tests passed successfully!"
else
    print_error "Some tests failed. Exit code: $TEST_EXIT_CODE"
fi

# Show test results if available
if [[ -d "test-results" ]] && [[ -n "$(ls -A test-results 2>/dev/null)" ]]; then
    print_status "Test results saved in: test-results/"
    ls -la test-results/
fi

echo ""
print_status "Simulation logs and test output are available via:"
print_status "  docker-compose logs simulator"
print_status "  docker-compose logs test-runner"

if [[ "$WITH_VIZ" == true ]]; then
    print_status "To run the visualizer again:"
    print_status "  docker-compose up visualizer"
fi

echo ""
echo "=================================================================="
print_status "SIL-Harness test run complete."
echo "=================================================================="

exit $TEST_EXIT_CODE
