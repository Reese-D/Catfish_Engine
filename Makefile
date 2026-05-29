# Basic Makefile for Catfish Engine

# Compiler
CXX = clang++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -ggdb
LDFLAGS = -lvulkan

# Target executable name
TARGET = catfish_engine

# Source files
SOURCES = src/engine.cpp src/window.cpp src/vulkan_instance.cpp src/validation_layers.cpp src/physical_device.cpp src/logical_device.cpp src/surface.cpp src/swap_chain.cpp src/graphics_pipeline.cpp src/renderer.cpp src/vulkan_utils.cpp src/vertex_buffer.cpp src/index_buffer.cpp src/uniform_buffer.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

SHADER_SRC = shaders/shader.slang
VERT_SPV = shaders/vert.spv
FRAG_SPV = shaders/frag.spv
SLANGC_FLAGS = -target spirv -matrix-layout-column-major

# Default target
all: shaders $(TARGET)

# Compile Slang shaders to SPIR-V
shaders: $(VERT_SPV) $(FRAG_SPV)

$(VERT_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry vertexMain $< -o $@

$(FRAG_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry fragmentMain $< -o $@

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS) -lglfw

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET) $(VERT_SPV) $(FRAG_SPV)

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
	clang-format -i $(SOURCES) src/surface.h src/swap_chain.h src/graphics_pipeline.h src/renderer.h src/vertex_buffer.h src/index_buffer.h src/uniform_buffer.h src/vulkan_utils.h

# Phony targets
.PHONY: all clean format shaders install uninstall local-install local-uninstall
