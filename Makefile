# Basic Makefile for Catfish Engine

# Compiler
CXX = clang++
CC  = clang

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -O0 -ggdb \
           -Ithird_party \
           -Ithird_party/fastgltf/include \
           -Ithird_party/entt \
           -Ithird_party/imgui \
           -Ithird_party/imgui/backends \
           -Ithird_party/enet/include \
           -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE

CFLAGS = -O2 -Ithird_party/enet/include

LDFLAGS = -lvulkan

# Target executable name
TARGET = catfish_engine

# C++ source files
SOURCES = \
  src/main.cpp src/engine.cpp src/rts_game.cpp \
  src/window.cpp src/vulkan_instance.cpp src/validation_layers.cpp \
  src/physical_device.cpp src/logical_device.cpp src/surface.cpp \
  src/swap_chain.cpp src/graphics_pipeline.cpp src/renderer.cpp \
  src/vulkan_utils.cpp src/vertex_buffer.cpp src/index_buffer.cpp \
  src/uniform_buffer.cpp src/texture_image.cpp src/depth_buffer.cpp \
  src/model.cpp src/material.cpp src/terrain.cpp src/selection_ring.cpp \
  src/camera_system.cpp src/render_system.cpp src/order_system.cpp \
  src/input_system.cpp src/selection_system.cpp src/spatial_grid.cpp \
  src/combat_system.cpp src/movement_system.cpp src/hud_system.cpp \
  src/menu_system.cpp src/pathfinder.cpp \
  src/fog_of_war.cpp src/fog_system.cpp src/minimap_system.cpp \
  src/projectile_system.cpp src/lava_zone.cpp src/lava_system.cpp \
  src/death_system.cpp src/network_manager.cpp src/headless_runner.cpp \
  third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp \
  third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp \
  third_party/imgui/backends/imgui_impl_glfw.cpp \
  third_party/imgui/backends/imgui_impl_vulkan.cpp

# ENet C sources (compiled with CC, not CXX)
ENET_SRCS = \
  third_party/enet/callbacks.c \
  third_party/enet/compress.c \
  third_party/enet/host.c \
  third_party/enet/list.c \
  third_party/enet/packet.c \
  third_party/enet/peer.c \
  third_party/enet/protocol.c \
  third_party/enet/unix.c

OBJECTS      = $(SOURCES:.cpp=.o)
ENET_OBJECTS = $(ENET_SRCS:.c=.o)

SHADER_SRC  = shaders/shader.slang
VERT_SPV    = shaders/vert.spv
FRAG_SPV    = shaders/frag.spv
SLANGC_FLAGS = -target spirv -matrix-layout-column-major

# Default target
all: shaders $(TARGET)

# Compile Slang shaders to SPIR-V
shaders: $(VERT_SPV) $(FRAG_SPV)

$(VERT_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry vertexMain $< -o $@

$(FRAG_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry fragmentMain $< -o $@

# Link
$(TARGET): $(OBJECTS) $(ENET_OBJECTS)
	$(CXX) $(OBJECTS) $(ENET_OBJECTS) -o $(TARGET) $(LDFLAGS) \
	  -lglfw third_party/fastgltf/libfastgltf.a -lsimdjson

# Compile C++ sources
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ENet C sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(ENET_OBJECTS) $(TARGET) $(VERT_SPV) $(FRAG_SPV)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

local-install: $(TARGET)
	install -m 755 $(TARGET) ./bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

local-uninstall:
	rm -f ./bin/$(TARGET)

format:
	clang-format -i src/*.cpp src/*.h

.PHONY: all clean format shaders install uninstall local-install local-uninstall
