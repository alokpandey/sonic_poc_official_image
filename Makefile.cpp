# SONiC C++ Functional Tests Makefile

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -Isrc/cpp
LIBS = -lpthread

# Directories
SRC_DIR = src/cpp
BUILD_DIR = build
HAL_DIR = $(SRC_DIR)/hal
SAI_DIR = $(SRC_DIR)/sai
INTERRUPT_DIR = $(SRC_DIR)/interrupts
TESTS_DIR = $(SRC_DIR)/tests

# Source files
HAL_SOURCES = $(HAL_DIR)/sonic_hal_controller.cpp
SAI_SOURCES = $(SAI_DIR)/sonic_sai_controller.cpp
INTERRUPT_SOURCES = $(INTERRUPT_DIR)/sonic_interrupt_controller.cpp
TEST_SOURCES = $(TESTS_DIR)/sonic_functional_tests.cpp
MAIN_SOURCE = $(TESTS_DIR)/main_test_runner.cpp

# Object files
HAL_OBJECTS = $(BUILD_DIR)/sonic_hal_controller.o
SAI_OBJECTS = $(BUILD_DIR)/sonic_sai_controller.o
INTERRUPT_OBJECTS = $(BUILD_DIR)/sonic_interrupt_controller.o
TEST_OBJECTS = $(BUILD_DIR)/sonic_functional_tests.o
MAIN_OBJECT = $(BUILD_DIR)/main_test_runner.o

# Target executable
TARGET = $(BUILD_DIR)/sonic_functional_tests

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile HAL controller
$(HAL_OBJECTS): $(HAL_SOURCES) $(HAL_DIR)/sonic_hal_controller.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(HAL_SOURCES) -o $(HAL_OBJECTS)

# Compile SAI controller
$(SAI_OBJECTS): $(SAI_SOURCES) $(SAI_DIR)/sonic_sai_controller.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SAI_SOURCES) -o $(SAI_OBJECTS)

# Compile Interrupt controller
$(INTERRUPT_OBJECTS): $(INTERRUPT_SOURCES) $(INTERRUPT_DIR)/sonic_interrupt_controller.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(INTERRUPT_SOURCES) -o $(INTERRUPT_OBJECTS)

# Compile test framework
$(TEST_OBJECTS): $(TEST_SOURCES) $(TESTS_DIR)/sonic_functional_tests.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(TEST_SOURCES) -o $(TEST_OBJECTS)

# Compile main test runner
$(MAIN_OBJECT): $(MAIN_SOURCE) $(TESTS_DIR)/sonic_functional_tests.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(MAIN_SOURCE) -o $(MAIN_OBJECT)

# Link final executable
$(TARGET): $(HAL_OBJECTS) $(SAI_OBJECTS) $(INTERRUPT_OBJECTS) $(TEST_OBJECTS) $(MAIN_OBJECT)
	$(CXX) $(CXXFLAGS) $(HAL_OBJECTS) $(SAI_OBJECTS) $(INTERRUPT_OBJECTS) $(TEST_OBJECTS) $(MAIN_OBJECT) $(LIBS) -o $(TARGET)
	@echo "Build completed successfully!"
	@echo "Executable: $(TARGET)"

# Individual component builds
hal: $(HAL_OBJECTS)
	@echo "HAL controller compiled successfully"

sai: $(SAI_OBJECTS)
	@echo "SAI controller compiled successfully"

interrupts: $(INTERRUPT_OBJECTS)
	@echo "Interrupt controller compiled successfully"

tests: $(TEST_OBJECTS)
	@echo "Test framework compiled successfully"

# Debug build
debug: CXXFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)
	@echo "Debug build completed"

# Release build
release: CXXFLAGS += -DNDEBUG -O3
release: $(TARGET)
	@echo "Release build completed"

# Static analysis
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c++17 $(SRC_DIR) 2> static_analysis.txt
	@echo "Static analysis completed. Results in static_analysis.txt"

# Code formatting
format:
	@echo "Formatting code..."
	find $(SRC_DIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatting completed"

# Documentation generation
docs:
	@echo "Generating documentation..."
	doxygen Doxyfile 2>/dev/null || echo "Doxygen not available"

# Test execution targets
run-tests: $(TARGET)
	@echo "Running all functional tests..."
	./$(TARGET) --verbose

run-quick: $(TARGET)
	@echo "Running quick test suite..."
	./$(TARGET) --quick --verbose

run-hal: $(TARGET)
	@echo "Running HAL tests only..."
	./$(TARGET) --hal-only --verbose

run-sai: $(TARGET)
	@echo "Running SAI tests only..."
	./$(TARGET) --sai-only --verbose

run-integration: $(TARGET)
	@echo "Running integration tests..."
	./$(TARGET) --integration-only --verbose

run-stress: $(TARGET)
	@echo "Running stress tests..."
	./$(TARGET) --stress-tests --verbose --timeout 300

# Test with different configurations
test-stop-on-failure: $(TARGET)
	./$(TARGET) --verbose --stop-on-failure

test-quiet: $(TARGET)
	./$(TARGET) --quiet

test-with-output: $(TARGET)
	./$(TARGET) --verbose --output test_results_$(shell date +%Y%m%d_%H%M%S).txt

# Validation targets
validate-hal: $(TARGET)
	@echo "Validating HAL functionality..."
	./$(TARGET) --hal-only --stop-on-failure --verbose

validate-sai: $(TARGET)
	@echo "Validating SAI functionality..."
	./$(TARGET) --sai-only --stop-on-failure --verbose

validate-all: $(TARGET)
	@echo "Validating complete system..."
	./$(TARGET) --stop-on-failure --verbose

# Performance testing
benchmark: $(TARGET)
	@echo "Running performance benchmarks..."
	time ./$(TARGET) --stress-tests --quiet

# Memory testing (requires valgrind)
memcheck: $(TARGET)
	@echo "Running memory check..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(TARGET) --quick --quiet

# Coverage testing (requires gcov)
coverage: CXXFLAGS += --coverage
coverage: $(TARGET)
	@echo "Running coverage test..."
	./$(TARGET) --quick --quiet
	gcov $(HAL_SOURCES) $(SAI_SOURCES) $(TEST_SOURCES)
	@echo "Coverage report generated"

# Installation
install: $(TARGET)
	@echo "Installing SONiC functional tests..."
	cp $(TARGET) /usr/local/bin/sonic_functional_tests
	chmod +x /usr/local/bin/sonic_functional_tests
	@echo "Installation completed"

# Uninstallation
uninstall:
	@echo "Uninstalling SONiC functional tests..."
	rm -f /usr/local/bin/sonic_functional_tests
	@echo "Uninstallation completed"

# Clean targets
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f static_analysis.txt
	rm -f *.gcov *.gcda *.gcno
	@echo "Clean completed"

clean-all: clean
	@echo "Cleaning all generated files..."
	rm -f test_results_*.txt
	rm -rf docs/
	@echo "Deep clean completed"

# Help target
help:
	@echo "SONiC C++ Functional Tests Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  all          - Build complete test suite (default)"
	@echo "  hal          - Build HAL controller only"
	@echo "  sai          - Build SAI controller only"
	@echo "  tests        - Build test framework only"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo ""
	@echo "Test execution:"
	@echo "  run-tests    - Run all functional tests"
	@echo "  run-quick    - Run quick test suite"
	@echo "  run-hal      - Run HAL tests only"
	@echo "  run-sai      - Run SAI tests only"
	@echo "  run-integration - Run integration tests"
	@echo "  run-stress   - Run stress tests"
	@echo ""
	@echo "Validation:"
	@echo "  validate-hal - Validate HAL functionality"
	@echo "  validate-sai - Validate SAI functionality"
	@echo "  validate-all - Validate complete system"
	@echo ""
	@echo "Quality assurance:"
	@echo "  analyze      - Run static analysis"
	@echo "  format       - Format source code"
	@echo "  memcheck     - Run memory leak detection"
	@echo "  coverage     - Generate code coverage report"
	@echo "  benchmark    - Run performance benchmarks"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-all    - Remove all generated files"
	@echo "  install      - Install to system"
	@echo "  uninstall    - Remove from system"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all hal sai tests debug release analyze format docs clean clean-all help
.PHONY: run-tests run-quick run-hal run-sai run-integration run-stress
.PHONY: test-stop-on-failure test-quiet test-with-output
.PHONY: validate-hal validate-sai validate-all benchmark memcheck coverage
.PHONY: install uninstall

# Default target
.DEFAULT_GOAL := all
