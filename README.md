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
sudo pacman -S shader-slang clang21 llvm21 vulkan-headers vulkan-icd-loader vulkan-validation-layers glfw simdjson
```
`slangc` is available via the `shader-slang` AUR package or the upstream release tarball.

> **Note:** The build uses `clang++` and `clang` from `/usr/lib/llvm21/bin/`. The versioned `clang21` / `llvm21` packages are required — the system clang (22+) is incompatible with the vendored clang-uml build.

**Ubuntu / Debian:**
```sh
sudo apt install clang-21 llvm-21 libvulkan-dev vulkan-validationlayers-dev libglfw3-dev libsimdjson-dev
```
Install `slangc` from the [Slang GitHub releases](https://github.com/shader-slang/slang/releases) and place it on your `PATH`.

**Optional (diagram rendering only):**

| Package | Notes |
|---|---|
| `plantuml` | Renders `.puml` files produced by clang-uml |
| `graphviz` | Required by PlantUML for layout |

```sh
# Arch Linux
sudo pacman -S plantuml graphviz

# Ubuntu / Debian
sudo apt install plantuml graphviz
```

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

## Code quality

Three tools are wired into the Makefile. All require `clang21` / `llvm21` to be installed (see [Dependencies](#dependencies)).

### clang-format

Formats all project sources and headers in-place using the rules in `.clang-format` (LLVM style, 4-space indent, 185-column limit):

```sh
make format
```

### clang-tidy

Checks naming conventions and code quality issues defined in `.clang-tidy`. Reports only, no changes:

```sh
make tidy
```

To automatically fix naming violations (uses `run-clang-tidy` for atomic cross-TU renames):

```sh
make tidy-fix
```

### clang-uml and PlantUML

Diagram generation is a two-step process and is **not invoked by the default `make` target**.

**Step 1** — generate `.puml` source files from the C++ AST (requires clang-uml):

```sh
make uml
```

**Step 2** — render the `.puml` files to SVG (requires `plantuml` and `graphviz`):

```sh
make diagrams
```

Output files (one per diagram defined in `.clang-uml`):

| File | Contents |
|---|---|
| `diagrams/vulkan_stack.puml` | `VulkanHelpers` device/pipeline class hierarchy |
| `diagrams/game_classes.puml` | Game abstraction layer — runners, interfaces, implementations |
| `diagrams/ecs.puml` | ECS components and systems |
| `diagrams/networking.puml` | `Network::NetworkManager` and message structs |
| `diagrams/packages.puml` | Top-level namespace package overview |

### Building clang-uml

clang-uml must be built from source against LLVM 21 (the upstream package requires `clang < 22`). A PKGBUILD is provided at `clang-uml/PKGBUILD` for Arch-based systems:

```sh
cd clang-uml
makepkg -si
```

This downloads the clang-uml 0.6.2 source, builds it against the installed `llvm21` / `clang21`, and installs `/usr/bin/clang-uml`.

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
