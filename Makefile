# Makefile for Integrated AI Synthesis System
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = 
LIBS = 

# Source file
SRC = integrated_ai_synthesis_system.cpp

# Output executable
TARGET = ai_synthesis

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SRC) $(LIBS)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Install (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)

# Release build
release: CXXFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Check for memory leaks (requires valgrind)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Static analysis (requires cppcheck)
analyze:
	cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem $(SRC)

# Format code (requires clang-format)
format:
	clang-format -i $(SRC)

# Show help
help:
	@echo "Available targets:"
	@echo "  all      - Build the program (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  run      - Build and run the program"
	@echo "  debug    - Build with debug flags"
	@echo "  release  - Build with optimization flags"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  memcheck - Run with valgrind memory checker"
	@echo "  analyze  - Run static code analysis"
	@echo "  format   - Format code with clang-format"
	@echo "  help     - Show this help message"

.PHONY: all clean install uninstall run debug release memcheck analyze format help