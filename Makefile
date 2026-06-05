# Catfish Engine — split server/client build

CXX          = /usr/lib/llvm21/bin/clang++
CC           = /usr/lib/llvm21/bin/clang-21
CLANG_TIDY   = /usr/lib/llvm21/bin/clang-tidy
CLANG_FORMAT = /usr/lib/llvm21/bin/clang-format

all: catfish_server catfish_client

# ---- Include paths -----------------------------------------------------------

SHARED_INC = \
  -Iinclude/shared/ecs \
  -Iinclude/shared/game \
  -Iinclude/shared/network \
  -Ithird_party \
  -Ithird_party/entt \
  -Ithird_party/enet/include \
  -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE

SERVER_INC = $(SHARED_INC) \
  -Iinclude/server/game

CLIENT_INC = $(SHARED_INC) \
  -Iinclude/client/engine \
  -Iinclude/client/ecs \
  -Iinclude/client/game \
  -Ithird_party/fastgltf/include \
  -Ithird_party/imgui \
  -Ithird_party/imgui/backends

BASEFLAGS = -std=c++23 -Wall -Wextra -O0 -ggdb
CFLAGS    = -O2 -Ithird_party/enet/include

# ---- ENet C sources (shared by both binaries) --------------------------------

ENET_SRCS = \
  third_party/enet/callbacks.c \
  third_party/enet/compress.c \
  third_party/enet/host.c \
  third_party/enet/list.c \
  third_party/enet/packet.c \
  third_party/enet/peer.c \
  third_party/enet/protocol.c \
  third_party/enet/unix.c

ENET_OBJS = $(ENET_SRCS:.c=.o)

# ---- Shared static library ---------------------------------------------------

SHARED_SRCS = \
  src/shared/ecs/movement_system.cpp \
  src/shared/ecs/combat_system.cpp \
  src/shared/ecs/death_system.cpp \
  src/shared/ecs/order_system.cpp \
  src/shared/ecs/pathfinder.cpp \
  src/shared/ecs/fog_of_war.cpp \
  src/shared/ecs/lava_zone.cpp \
  src/shared/ecs/lava_sim.cpp \
  src/shared/ecs/projectile_sim.cpp \
  src/shared/ecs/spatial_grid.cpp \
  src/shared/game/rts_game_base.cpp \
  src/shared/game/headless_runner.cpp \
  src/shared/network/network_manager.cpp

SHARED_OBJS = $(SHARED_SRCS:.cpp=.o)

libcatfish_shared.a: $(SHARED_OBJS)
	ar rcs $@ $^

$(SHARED_OBJS): %.o: %.cpp
	$(CXX) $(BASEFLAGS) $(SHARED_INC) -c $< -o $@

# ---- Server binary -----------------------------------------------------------

SERVER_SRCS = \
  src/server/main.cpp \
  src/server/game/rts_game_server.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)

catfish_server: $(SERVER_OBJS) libcatfish_shared.a $(ENET_OBJS)
	$(CXX) $(SERVER_OBJS) libcatfish_shared.a $(ENET_OBJS) -o $@

$(SERVER_OBJS): %.o: %.cpp
	$(CXX) $(BASEFLAGS) $(SERVER_INC) -c $< -o $@

# ---- Client binary -----------------------------------------------------------

CLIENT_SRCS = \
  src/client/main.cpp \
  src/client/game/rts_game_client.cpp \
  src/client/game/menu_system.cpp \
  src/client/ecs/render_system.cpp \
  src/client/ecs/camera_system.cpp \
  src/client/ecs/input_system.cpp \
  src/client/ecs/selection_system.cpp \
  src/client/ecs/hud_system.cpp \
  src/client/ecs/fog_system.cpp \
  src/client/ecs/minimap_system.cpp \
  src/client/ecs/projectile_render.cpp \
  src/client/ecs/lava_render.cpp \
  src/client/engine/engine.cpp \
  src/client/engine/window.cpp \
  src/client/engine/vulkan_instance.cpp \
  src/client/engine/validation_layers.cpp \
  src/client/engine/physical_device.cpp \
  src/client/engine/logical_device.cpp \
  src/client/engine/surface.cpp \
  src/client/engine/swap_chain.cpp \
  src/client/engine/graphics_pipeline.cpp \
  src/client/engine/renderer.cpp \
  src/client/engine/vulkan_utils.cpp \
  src/client/engine/vertex_buffer.cpp \
  src/client/engine/index_buffer.cpp \
  src/client/engine/uniform_buffer.cpp \
  src/client/engine/texture_image.cpp \
  src/client/engine/depth_buffer.cpp \
  src/client/engine/model.cpp \
  src/client/engine/material.cpp \
  src/client/engine/terrain.cpp \
  src/client/engine/selection_ring.cpp \
  third_party/imgui/imgui.cpp \
  third_party/imgui/imgui_draw.cpp \
  third_party/imgui/imgui_tables.cpp \
  third_party/imgui/imgui_widgets.cpp \
  third_party/imgui/backends/imgui_impl_glfw.cpp \
  third_party/imgui/backends/imgui_impl_vulkan.cpp

CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Default: dynamic linking for local dev. CI overrides this with static flags.
CLIENT_LIBS = -lvulkan -lglfw third_party/fastgltf/libfastgltf.a -lsimdjson

catfish_client: shaders $(CLIENT_OBJS) libcatfish_shared.a $(ENET_OBJS)
	$(CXX) $(CLIENT_OBJS) libcatfish_shared.a $(ENET_OBJS) -o $@ $(CLIENT_LIBS)

$(CLIENT_OBJS): %.o: %.cpp
	$(CXX) $(BASEFLAGS) $(CLIENT_INC) -c $< -o $@

# ---- Shaders -----------------------------------------------------------------

SHADER_SRC   = shaders/shader.slang
VERT_SPV     = shaders/vert.spv
FRAG_SPV     = shaders/frag.spv
SLANGC_FLAGS = -target spirv -matrix-layout-column-major

shaders: $(VERT_SPV) $(FRAG_SPV)

$(VERT_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry vertexMain $< -o $@

$(FRAG_SPV): $(SHADER_SRC)
	slangc $(SLANGC_FLAGS) -entry fragmentMain $< -o $@

# ---- ENet C compile ----------------------------------------------------------

$(ENET_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Phony targets -----------------------------------------------------------

clean:
	rm -f $(SHARED_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) $(ENET_OBJS) \
	      libcatfish_shared.a catfish_server catfish_client \
	      $(VERT_SPV) $(FRAG_SPV)

# Project sources only (no third_party) — used by tidy and format
PROJECT_SRCS = $(SHARED_SRCS) $(SERVER_SRCS) \
  $(filter-out third_party/%,$(CLIENT_SRCS))

PROJECT_FILES = $(shell find src include -name '*.cpp' -o -name '*.h' \
  | grep -v third_party)

# ---- Code quality targets ----------------------------------------------------

# Run all enabled clang-tidy checks (report only)
tidy:
	$(CLANG_TIDY) $(PROJECT_SRCS)

# Fix naming violations atomically across all translation units
tidy-fix:
	run-clang-tidy -clang-tidy-binary $(CLANG_TIDY) \
	  -checks='-*,readability-identifier-naming' -fix \
	  'src/.*\.cpp'

# Apply clang-format in-place to all project sources and headers
format:
	$(CLANG_FORMAT) -i $(PROJECT_FILES)

# Generate .puml files into diagrams/
uml:
	clang-uml

# Render .puml files to SVG (requires plantuml + graphviz)
diagrams:
	plantuml diagrams/*.puml

local-install: catfish_server catfish_client
	install -m 755 catfish_server catfish_client ./bin/

local-uninstall:
	rm -f ./bin/catfish_server ./bin/catfish_client

.PHONY: all clean tidy tidy-fix format uml diagrams shaders local-install local-uninstall
