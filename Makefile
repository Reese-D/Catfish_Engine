# Basic Makefile for Catfish Engine

# Compiler
CXX = clang++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -ggdb -Ithird_party -Ithird_party/fastgltf/include -Ithird_party/entt -Ithird_party/imgui -Ithird_party/imgui/backends -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE
LDFLAGS = -lvulkan

# Target executable name
TARGET = catfish_engine

# Source files
SOURCES = src/engine.cpp src/window.cpp src/vulkan_instance.cpp src/validation_layers.cpp src/physical_device.cpp src/logical_device.cpp src/surface.cpp src/swap_chain.cpp src/graphics_pipeline.cpp src/renderer.cpp src/vulkan_utils.cpp src/vertex_buffer.cpp src/index_buffer.cpp src/uniform_buffer.cpp src/texture_image.cpp src/depth_buffer.cpp src/model.cpp src/material.cpp src/terrain.cpp src/selection_ring.cpp src/camera_system.cpp src/render_system.cpp src/order_system.cpp src/input_system.cpp src/selection_system.cpp src/spatial_grid.cpp src/combat_system.cpp src/movement_system.cpp src/hud_system.cpp src/menu_system.cpp src/pathfinder.cpp src/fog_of_war.cpp src/fog_system.cpp third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp third_party/imgui/backends/imgui_impl_glfw.cpp third_party/imgui/backends/imgui_impl_vulkan.cpp

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
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS) -lglfw third_party/fastgltf/libfastgltf.a -lsimdjson

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
	clang-format -i src/*.cpp src/*.h

# Phony targets
.PHONY: all clean format shaders install uninstall local-install local-uninstall
