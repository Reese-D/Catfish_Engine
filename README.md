# Catfish Engine

A Vulkan 1.3 renderer with an EnTT ECS layer, built into a simple real-time strategy game. Features fog of war, a minimap, projectiles, lava, and a networking layer with separate server and client binaries.

## Dependencies

Install these via your package manager before building.

**Required:**

| Package | Notes |
|---|---|
| `clang` | C++23, used for both `clang++` and `clang` |
| `vulkan-headers` / `vulkan-devel` | Vulkan SDK headers (client only) |
| `libvulkan` | Vulkan loader — `libvulkan.so` (client only) |
| `vulkan-validation-layers` | Optional but recommended during development |
| `glfw` | Windowing — `libglfw` (client only) |
| `simdjson` | Required by fastgltf — `libsimdjson` (client only) |
| `slang` | Shader compiler — `slangc` must be on `PATH` (client only) |

**Arch Linux:**
```sh
sudo pacman -S clang vulkan-headers vulkan-icd-loader vulkan-validation-layers glfw simdjson
```
`slangc` is available via the `shader-slang` AUR package or the upstream release tarball.

**Ubuntu / Debian:**
```sh
sudo apt install clang libvulkan-dev vulkan-validationlayers-dev libglfw3-dev libsimdjson-dev
```
Install `slangc` from the [Slang GitHub releases](https://github.com/shader-slang/slang/releases) and place it on your `PATH`.

**Vendored (no action needed):**
- EnTT, Dear ImGui, fastgltf, ENet, stb_image — all bundled under `third_party/`

## Build

```sh
git clone <repo-url>
cd Catfish_Engine
make          # builds both catfish_server and catfish_client
make catfish_server   # server only (no Vulkan/GLFW required)
make catfish_client   # client only
make clean    # remove build artefacts and SPIR-V blobs
```

The server binary has no Vulkan dependency and can be built and run on headless machines.

## Running

Start the server first, then connect one or more clients.

**Server** (headless, no window):
```sh
./catfish_server [--port <port>]
# default port: 1234
```

**Client:**
```sh
./catfish_client --host <server-address> [--port <port>]
# e.g. ./catfish_client --host 127.0.0.1
#      ./catfish_client --host 192.168.1.10 --port 5000
```

The first client to connect is assigned the Player faction; the second gets Enemy. Each client sees fog of war from their own units' perspective. Up to four clients can connect simultaneously.

## Controls

| Input | Action |
|---|---|
| Right-click | Set thrust direction (unit accelerates toward cursor continuously) |
| Q | Fire fast projectile toward cursor |
| E | Launch gravity well toward cursor (pulls units in after 0.5 s) |
| Left-click | Select unit |
| Middle-drag / scroll | Pan / zoom camera |
| Left-click minimap | Pan camera to location |

## Project structure

```
include/
  shared/      Headers used by both binaries
    ecs/         Simulation component and system headers
    game/        Shared game base, server game interface, HeadlessRunner
    network/     NetworkManager, message structs, protocol version
  server/      Server-only headers
    game/        RtsGameServer
  client/      Client-only headers
    engine/      Vulkan backend (VulkanHelpers namespace)
    ecs/         Rendering and input system headers; RenderMesh component
    game/        RtsGameClient, MenuSystem
src/
  shared/      Compiled into libcatfish_shared.a (linked by both binaries)
    ecs/         Simulation systems (movement, combat, projectiles, lava, etc.)
    game/        RtsGameBase, HeadlessRunner
    network/     NetworkManager
  server/      catfish_server entry point and server game logic
  client/      catfish_client entry point, rendering, input, client game logic
shaders/       Slang shader source + compiled SPIR-V (client only)
models/        GLB model assets (client only)
third_party/   Vendored libraries (EnTT, ImGui, fastgltf, ENet, stb_image)
Makefile
```
