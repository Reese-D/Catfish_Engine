# Basic Makefile for Catfish Engine

# Compiler
CXX = clang++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -O2
LDFLAGS = -lvulkan

# Target executable name
TARGET = catfish_engine

# Source files
SOURCES = src/engine.cpp src/window.cpp src/vulkan_instance.cpp src/validation_layers.cpp src/physical_device.cpp src/logical_device.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS) -lglfw

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install target (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Local install target for testing
local-install: $(TARGET)
	install -m 755 $(TARGET) ./bin/

# Uninstall target (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Local uninstall target
local-uninstall:
	rm -f ./bin/$(TARGET)

# Format source files with clang-format
format:
	clang-format -i $(SOURCES)

# Phony targets
.PHONY: all clean format install uninstall local-install local-uninstall