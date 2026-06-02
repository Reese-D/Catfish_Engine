# Catfish Engine

A Vulkan 1.3 renderer with an EnTT ECS layer, built into a simple real-time strategy game. Features pathfinding, fog of war, a minimap, projectiles, lava, and a basic networking layer (dedicated server + client).

## Dependencies

Install these via your package manager before building.

**Required:**

| Package | Notes |
|---|---|
| `clang` | C++23, used for both `clang++` and `clang` |
| `vulkan-headers` / `vulkan-devel` | Vulkan SDK headers |
| `libvulkan` | Vulkan loader (`libvulkan.so`) |
| `vulkan-validation-layers` | Optional but recommended during development |
| `glfw` | Windowing (`libglfw`) |
| `simdjson` | Required by fastgltf (`libsimdjson`) |
| `slang` | Shader compiler (`slangc` must be on `PATH`) |

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
make
```

This compiles the shaders via `slangc` then links the `catfish_engine` binary in the project root.

```sh
make clean   # remove build artefacts and SPIR-V blobs
```

## Running

### Standalone (local, single machine)

```sh
./catfish_engine
```

Both factions are controlled by the same player. Left-click to select a unit, right-click to move, Q to fire a projectile.

### Networked (two machines or two terminals)

**Server** (headless, no window):
```sh
./catfish_engine --server [port]
# default port: 1234
```

**Client:**
```sh
./catfish_engine --client <host> [port]
# e.g. ./catfish_engine --client 127.0.0.1
```

The first client to connect is assigned the Player faction; the second gets Enemy. Each client sees fog of war from their own units' perspective.

## Controls

| Input | Action |
|---|---|
| Left-click | Select unit |
| Right-click | Move selected unit |
| Q | Fire projectile toward cursor |
| Middle-drag / scroll | Pan / zoom camera |
| Left-click minimap | Pan camera to location |

## Project structure

```
src/           C++ sources and headers
shaders/       Slang shader source + compiled SPIR-V
models/        GLB model assets
third_party/   Vendored libraries (EnTT, ImGui, fastgltf, ENet, stb_image)
Makefile
```
