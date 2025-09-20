#!/bin/bash

# SONiC C++ Functional Test Runner Script
# This script builds and runs comprehensive HAL and SAI tests

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
COMPOSE_FILE="docker-compose-real-sonic.yml"
TEST_CONTAINER="sonic-cpp-tests"
SONIC_CONTAINER="sonic-vs-official"
RESULTS_DIR="./test_results"

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

print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS] [TEST_TYPE]

SONiC C++ Functional Test Runner - Comprehensive HAL and SAI Testing

TEST_TYPE:
    all             Run all functional tests (default)
    hal             Run HAL tests only (fan, temperature, PSU, LED, interfaces)
    sai             Run SAI tests only (VLAN, port config, FDB, routing)
    interrupt       Run interrupt tests only (cable events, link flaps, SFP hot swap)
    integration     Run integration tests
    stress          Run stress tests
    quick           Run quick test suite
    validate        Run validation tests

OPTIONS:
    -h, --help      Show this help message
    -v, --verbose   Enable verbose output
    -q, --quiet     Quiet mode (minimal output)
    -s, --stop      Stop on first failure
    -t, --timeout   Set timeout in seconds (default: 30)
    -o, --output    Save results to specific file
    -c, --clean     Clean up before running tests
    -b, --build     Force rebuild of test container
    -d, --debug     Enable debug mode
    --no-cleanup    Don't cleanup after tests

Examples:
    $0                          # Run all tests
    $0 hal --verbose            # Run HAL tests with verbose output
    $0 sai --stop --timeout 60  # Run SAI tests, stop on failure, 60s timeout
    $0 quick --clean            # Clean and run quick tests
    $0 stress --output stress_results.txt  # Run stress tests, save to file

Real Hardware Testing Examples:
    $0 hal                      # Test fan control, temperature monitoring, PSU
    $0 sai                      # Test VLAN creation, port configuration
    $0 interrupt                # Test cable insertion/removal, link flaps
    $0 integration              # Test complete network setup scenarios
    $0 validate                 # Validate all functionality

EOF
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check if Docker is running
    if ! docker info >/dev/null 2>&1; then
        print_error "Docker is not running. Please start Docker first."
        exit 1
    fi
    
    # Check if docker-compose is available
    if ! command -v docker-compose >/dev/null 2>&1; then
        print_error "docker-compose is not installed. Please install docker-compose."
        exit 1
    fi
    
    # Check if compose file exists
    if [[ ! -f "$COMPOSE_FILE" ]]; then
        print_error "Docker compose file not found: $COMPOSE_FILE"
        exit 1
    fi
    
    print_success "Prerequisites check passed"
}

# Function to create results directory
setup_results_dir() {
    if [[ ! -d "$RESULTS_DIR" ]]; then
        print_status "Creating results directory: $RESULTS_DIR"
        mkdir -p "$RESULTS_DIR"
    fi
}

# Function to check if SONiC container is running
check_sonic_container() {
    print_status "Checking SONiC container status..."
    
    if ! docker ps | grep -q "$SONIC_CONTAINER"; then
        print_warning "SONiC container is not running. Starting SONiC infrastructure..."
        docker-compose -f "$COMPOSE_FILE" up -d sonic-vs-official sonic-sai-demo sonic-bsp-demo sonic-mgmt
        
        print_status "Waiting for SONiC container to be ready..."
        for i in {1..60}; do
            if docker exec "$SONIC_CONTAINER" redis-cli ping >/dev/null 2>&1; then
                print_success "SONiC container is ready!"
                break
            fi
            echo -n "."
            sleep 2
            if [[ $i -eq 60 ]]; then
                print_error "SONiC container failed to start properly"
                exit 1
            fi
        done
    else
        print_success "SONiC container is already running"
    fi
}

# Function to build test container
build_test_container() {
    print_status "Building C++ test container..."
    docker-compose -f "$COMPOSE_FILE" build sonic-cpp-tests
    print_success "Test container built successfully"
}

# Function to run tests
run_tests() {
    local test_type="$1"
    local test_args="$2"
    
    print_header "╔══════════════════════════════════════════════════════════════╗"
    print_header "║                 SONiC C++ Functional Tests                  ║"
    print_header "║              Hardware Abstraction Layer (HAL)               ║"
    print_header "║            Switch Abstraction Interface (SAI)               ║"
    print_header "╚══════════════════════════════════════════════════════════════╝"
    
    print_status "Starting $test_type tests..."
    print_status "Test arguments: $test_args"
    
    # Generate timestamp for results
    timestamp=$(date +"%Y%m%d_%H%M%S")
    result_file="$RESULTS_DIR/sonic_cpp_${test_type}_${timestamp}.txt"
    
    # Run the tests
    print_status "Executing tests in container..."
    if docker-compose -f "$COMPOSE_FILE" run --rm sonic-cpp-tests ./run_tests.sh $test_args --output "/sonic_tests/results/sonic_cpp_${test_type}_${timestamp}.txt"; then
        print_success "Tests completed successfully!"
        test_result=0
    else
        print_error "Tests failed!"
        test_result=1
    fi
    
    # Show results location
    if [[ -f "$result_file" ]]; then
        print_status "Test results saved to: $result_file"
        
        # Show summary if file exists and is readable
        if [[ -r "$result_file" ]]; then
            print_status "Test Summary:"
            echo "----------------------------------------"
            tail -20 "$result_file" | head -10
            echo "----------------------------------------"
        fi
    fi
    
    return $test_result
}

# Function to cleanup
cleanup() {
    if [[ "$NO_CLEANUP" != "true" ]]; then
        print_status "Cleaning up test containers..."
        docker-compose -f "$COMPOSE_FILE" down sonic-cpp-tests 2>/dev/null || true
    fi
}

# Function to show test results
show_results() {
    print_status "Recent test results:"
    if [[ -d "$RESULTS_DIR" ]]; then
        ls -la "$RESULTS_DIR"/*.txt 2>/dev/null | tail -5 || print_warning "No test result files found"
    fi
}

# Main function
main() {
    # Default values
    TEST_TYPE="all"
    VERBOSE=""
    QUIET=""
    STOP_ON_FAILURE=""
    TIMEOUT=""
    OUTPUT_FILE=""
    CLEAN=""
    BUILD=""
    DEBUG=""
    NO_CLEANUP=""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE="--verbose"
                shift
                ;;
            -q|--quiet)
                QUIET="--quiet"
                shift
                ;;
            -s|--stop)
                STOP_ON_FAILURE="--stop-on-failure"
                shift
                ;;
            -t|--timeout)
                TIMEOUT="--timeout $2"
                shift 2
                ;;
            -o|--output)
                OUTPUT_FILE="--output $2"
                shift 2
                ;;
            -c|--clean)
                CLEAN="true"
                shift
                ;;
            -b|--build)
                BUILD="true"
                shift
                ;;
            -d|--debug)
                DEBUG="true"
                set -x  # Enable debug mode
                shift
                ;;
            --no-cleanup)
                NO_CLEANUP="true"
                shift
                ;;
            all|hal|sai|interrupt|integration|stress|quick|validate)
                TEST_TYPE="$1"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Build test arguments
    TEST_ARGS=""
    case "$TEST_TYPE" in
        all)
            TEST_ARGS=""
            ;;
        hal)
            TEST_ARGS="--hal-only"
            ;;
        sai)
            TEST_ARGS="--sai-only"
            ;;
        interrupt)
            TEST_ARGS="--interrupt-only"
            ;;
        integration)
            TEST_ARGS="--integration-only"
            ;;
        stress)
            TEST_ARGS="--stress-tests"
            ;;
        quick)
            TEST_ARGS="--quick"
            ;;
        validate)
            TEST_ARGS="--stop-on-failure"
            ;;
    esac
    
    # Add additional arguments
    TEST_ARGS="$TEST_ARGS $VERBOSE $QUIET $STOP_ON_FAILURE $TIMEOUT $OUTPUT_FILE"
    
    # Trap cleanup on exit
    trap cleanup EXIT
    
    # Execute main workflow
    check_prerequisites
    setup_results_dir
    
    if [[ "$CLEAN" == "true" ]]; then
        print_status "Cleaning up previous containers..."
        docker-compose -f "$COMPOSE_FILE" down 2>/dev/null || true
        docker system prune -f >/dev/null 2>&1 || true
    fi
    
    check_sonic_container
    
    if [[ "$BUILD" == "true" ]] || ! docker images | grep -q "sonic-cpp-tests"; then
        build_test_container
    fi
    
    # Run the tests
    if run_tests "$TEST_TYPE" "$TEST_ARGS"; then
        print_success "All tests completed successfully!"
        show_results
        exit 0
    else
        print_error "Some tests failed!"
        show_results
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
