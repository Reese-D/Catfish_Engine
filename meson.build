project('catfish-engine', ['cpp', 'c'],
  version : '0.1.0',
  default_options : [
    'cpp_std=c++23',
    'warning_level=2',
    'buildtype=debug',
  ],
)

cpp      = meson.get_compiler('cpp')
glm_args = ['-DGLM_FORCE_RADIANS', '-DGLM_FORCE_DEPTH_ZERO_TO_ONE']

# ── System dependencies ───────────────────────────────────────────────────────

vulkan_dep   = dependency('vulkan')
glfw_dep     = dependency('glfw3')
simdjson_dep = dependency('simdjson', required: false)
if not simdjson_dep.found()
  simdjson_dep = dependency('libsimdjson')
endif
# GLM is header-only; libglm-dev installs into system include path, no pkg-config needed.

# ── Third-party: enet (vendored C source) ─────────────────────────────────────

enet_inc = include_directories('third_party/enet/include')

enet_lib = static_library('enet',
  sources : [
    'third_party/enet/callbacks.c',
    'third_party/enet/compress.c',
    'third_party/enet/host.c',
    'third_party/enet/list.c',
    'third_party/enet/packet.c',
    'third_party/enet/peer.c',
    'third_party/enet/protocol.c',
    'third_party/enet/unix.c',
  ],
  include_directories : enet_inc,
  c_args : ['-O2'],
)

enet_dep = declare_dependency(
  link_with : enet_lib,
  include_directories : enet_inc,
)

# ── Third-party: imgui (vendored C++ source) ──────────────────────────────────

imgui_inc = include_directories('third_party/imgui', 'third_party/imgui/backends')

imgui_lib = static_library('imgui',
  sources : [
    'third_party/imgui/imgui.cpp',
    'third_party/imgui/imgui_draw.cpp',
    'third_party/imgui/imgui_tables.cpp',
    'third_party/imgui/imgui_widgets.cpp',
    'third_party/imgui/backends/imgui_impl_glfw.cpp',
    'third_party/imgui/backends/imgui_impl_vulkan.cpp',
  ],
  include_directories : imgui_inc,
  dependencies : [vulkan_dep, glfw_dep],
)

imgui_dep = declare_dependency(
  link_with : imgui_lib,
  include_directories : imgui_inc,
)

# ── Third-party: fastgltf (pre-built static library) ──────────────────────────

fastgltf_lib = cpp.find_library('fastgltf',
  dirs : [meson.project_source_root() / 'third_party/fastgltf'],
  static : true,
)

fastgltf_dep = declare_dependency(
  dependencies : fastgltf_lib,
  include_directories : include_directories('third_party/fastgltf/include'),
)

# ── Third-party: header-only (EnTT, stb_image, GLM) ──────────────────────────

third_party_inc = include_directories('third_party', 'third_party/entt')

# ── Shared static library ─────────────────────────────────────────────────────

shared_inc = include_directories(
  'include/shared/ecs',
  'include/shared/game',
  'include/shared/network',
)

shared_lib = static_library('catfish_shared',
  sources : [
    'src/shared/ecs/movement_system.cpp',
    'src/shared/ecs/combat_system.cpp',
    'src/shared/ecs/death_system.cpp',
    'src/shared/ecs/order_system.cpp',
    'src/shared/ecs/pathfinder.cpp',
    'src/shared/ecs/fog_of_war.cpp',
    'src/shared/ecs/lava_zone.cpp',
    'src/shared/ecs/lava_sim.cpp',
    'src/shared/ecs/projectile_sim.cpp',
    'src/shared/ecs/spatial_grid.cpp',
    'src/shared/game/rts_game_base.cpp',
    'src/shared/game/headless_runner.cpp',
    'src/shared/network/network_manager.cpp',
  ],
  include_directories : [shared_inc, third_party_inc],
  dependencies : [enet_dep],
  cpp_args : glm_args,
)

shared_dep = declare_dependency(
  link_with : shared_lib,
  include_directories : [shared_inc, third_party_inc],
)

# ── Server binary ─────────────────────────────────────────────────────────────

executable('catfish_server',
  sources : [
    'src/server/main.cpp',
    'src/server/game/rts_game_server.cpp',
  ],
  include_directories : include_directories('include/server/game'),
  dependencies : [shared_dep, enet_dep],
  cpp_args : glm_args,
)

# ── Client binary ─────────────────────────────────────────────────────────────

client_inc = include_directories(
  'include/client/engine',
  'include/client/ecs',
  'include/client/game',
)

executable('catfish_client',
  sources : [
    'src/client/main.cpp',
    'src/client/game/rts_game_client.cpp',
    'src/client/game/menu_system.cpp',
    'src/client/ecs/render_system.cpp',
    'src/client/ecs/camera_system.cpp',
    'src/client/ecs/input_system.cpp',
    'src/client/ecs/selection_system.cpp',
    'src/client/ecs/hud_system.cpp',
    'src/client/ecs/fog_system.cpp',
    'src/client/ecs/minimap_system.cpp',
    'src/client/ecs/projectile_render.cpp',
    'src/client/ecs/lava_render.cpp',
    'src/client/engine/engine.cpp',
    'src/client/engine/window.cpp',
    'src/client/engine/vulkan_instance.cpp',
    'src/client/engine/validation_layers.cpp',
    'src/client/engine/physical_device.cpp',
    'src/client/engine/logical_device.cpp',
    'src/client/engine/surface.cpp',
    'src/client/engine/swap_chain.cpp',
    'src/client/engine/graphics_pipeline.cpp',
    'src/client/engine/projectile_pipeline.cpp',
    'src/client/engine/renderer.cpp',
    'src/client/engine/vulkan_utils.cpp',
    'src/client/engine/vertex_buffer.cpp',
    'src/client/engine/index_buffer.cpp',
    'src/client/engine/uniform_buffer.cpp',
    'src/client/engine/texture_image.cpp',
    'src/client/engine/depth_buffer.cpp',
    'src/client/engine/model.cpp',
    'src/client/engine/material.cpp',
    'src/client/engine/terrain.cpp',
    'src/client/engine/selection_ring.cpp',
  ],
  include_directories : [client_inc, shared_inc, third_party_inc],
  dependencies : [
    shared_dep,
    enet_dep,
    vulkan_dep,
    glfw_dep,
    simdjson_dep,
    fastgltf_dep,
    imgui_dep,
  ],
  cpp_args : glm_args,
)
