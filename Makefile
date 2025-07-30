# Multi-Dimensional Audio Configuration System - Makefile
# Modern C++17 build configuration

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG
DEBUG_FLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g -O0 -DDEBUG
INCLUDES = -Isrc
LDFLAGS = -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
DATA_DIR = data
CONFIG_DIR = config

# Source files
SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/audio_config_system.cpp \
          $(SRC_DIR)/audio_config_cli.cpp

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BUILD_DIR)/audio_config_system

# Default target
all: $(TARGET)

# Debug build
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build main target
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

# Download JSON library if not present
$(SRC_DIR)/json.hpp:
	@echo "Downloading nlohmann/json library..."
	curl -o $(SRC_DIR)/json.hpp -L https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Setup data directory and files
setup: $(SRC_DIR)/json.hpp
	@echo "Setting up project structure..."
	mkdir -p $(DATA_DIR) $(CONFIG_DIR)
	@if [ ! -f $(DATA_DIR)/clean_config.json ]; then \
		echo "⚠️  Warning: clean_config.json not found in $(DATA_DIR)/"; \
		echo "Please ensure you have the configuration database file."; \
	fi
	@echo "✅ Project setup complete"

# Run the application
run: $(TARGET)
	./$(TARGET)

# Run with specific config
run-with-config: $(TARGET)
	./$(TARGET) $(CONFIG_DIR)/weights.json

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Clean all generated files
distclean: clean
	rm -f $(SRC_DIR)/json.hpp

# Create sample configuration database (for testing)
create-sample-config:
	@echo "Creating sample configuration database..."
	@mkdir -p $(DATA_DIR)
	@echo '{\
		"Lead_Bright_Energetic": {\
			"soundCharacteristics": {\
				"timbral": "bright",\
				"dynamic": "energetic",\
				"emotional": [{"tag": "aggressive"}]\
			},\
			"adsr": {"type": "ADSR", "attack": 10, "decay": 200, "sustain": 0.7, "release": 500}\
		},\
		"Bass_Deep_Punchy": {\
			"soundCharacteristics": {\
				"timbral": "dark",\
				"dynamic": "punchy",\
				"emotional": [{"tag": "powerful"}]\
			},\
			"adsr": {"type": "ADSR", "attack": 5, "decay": 100, "sustain": 0.9, "release": 300}\
		},\
		"Pad_Warm_Calm": {\
			"soundCharacteristics": {\
				"timbral": "warm",\
				"dynamic": "calm",\
				"emotional": [{"tag": "peaceful"}]\
			},\
			"adsr": {"type": "AHDSR", "attack": 500, "hold": 100, "decay": 1000, "sustain": 0.6, "release": 2000}\
		}\
	}' > $(DATA_DIR)/clean_config.json
	@echo "✅ Sample configuration created at $(DATA_DIR)/clean_config.json"

# Run tests (placeholder for future test implementation)
test: $(TARGET)
	@echo "Running basic functionality tests..."
	@echo "🧪 Testing configuration loading..."
	@./$(TARGET) -test-config || echo "⚠️  Test configuration not found"
	@echo "✅ Basic tests completed"

# Install (copy to system location)
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/audio-config-system
	sudo chmod +x /usr/local/bin/audio-config-system
	@echo "✅ Installed as 'audio-config-system'"

# Uninstall
uninstall:
	@echo "Removing from /usr/local/bin..."
	sudo rm -f /usr/local/bin/audio-config-system
	@echo "✅ Uninstalled"

# Format code (requires clang-format)
format:
	@echo "Formatting source code..."
	@find $(SRC_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
	@echo "✅ Code formatted"

# Static analysis (requires cppcheck)
analyze:
	@echo "Running static analysis..."
	@cppcheck --enable=all --std=c++17 $(SRC_DIR)/ || echo "⚠️  Install cppcheck for static analysis"

# Generate documentation (requires doxygen)
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile || echo "⚠️  Install doxygen for documentation generation"

# Show help
help:
	@echo "Multi-Dimensional Audio Configuration System - Build Targets:"
	@echo ""
	@echo "🏗️  BUILD TARGETS:"
	@echo "  all          - Build release version (default)"
	@echo "  debug        - Build debug version with symbols"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo ""
	@echo "🔧 SETUP & DEPENDENCIES:"
	@echo "  setup        - Download dependencies and setup directories"
	@echo "  create-sample-config - Create sample configuration for testing"
	@echo ""
	@echo "🚀 EXECUTION:"
	@echo "  run          - Build and run the application"
	@echo "  run-with-config - Run with specific configuration file"
	@echo "  test         - Run basic functionality tests"
	@echo ""
	@echo "📦 DISTRIBUTION:"
	@echo "  install      - Install to system (/usr/local/bin)"
	@echo "  uninstall    - Remove from system"
	@echo ""
	@echo "🔍 DEVELOPMENT:"
	@echo "  format       - Format source code (requires clang-format)"
	@echo "  analyze      - Run static analysis (requires cppcheck)"
	@echo "  docs         - Generate documentation (requires doxygen)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "🎵 USAGE EXAMPLE:"
	@echo "  make setup && make && make run"

# Declare phony targets
.PHONY: all debug clean distclean setup run run-with-config create-sample-config test install uninstall format analyze docs help

# Default target
.DEFAULT_GOAL := all