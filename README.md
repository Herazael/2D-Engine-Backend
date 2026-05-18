# NeoLab2D Engine Backend

2D engine backend written in modern C++ with explicit architecture boundaries:

- Core layer defines app/scene/renderer contracts.
- Platform layer provides SDL3 + OpenGL implementations.
- Sandbox is the composition root used for running and testing the stack.

## Current State

- Window creation and app loop are running through `SdlAppHost`.
- OpenGL 3.3 context setup and rendering path are implemented in `SpriteRenderer`.
- Sprite data path (`loadTexture`, `drawSprite`, `buildAndFlushBatch`) is wired end-to-end.
- Sandbox boots `Application` with concrete platform adapters.

## Tech Stack

- C++26
- CMake 3.20+
- Ninja generator
- vcpkg (manifest mode)
- SDL3
- OpenGL 3.3 core profile
- GLAD2
- stb

## Prerequisites (Windows)

- Windows 10/11 (x64)
- Visual Studio 2022 Build Tools with Desktop C++ workload and Windows SDK
- CMake (3.20+)
- Ninja
- clang++ available on `PATH` (current preset sets `CMAKE_CXX_COMPILER=clang++`)
- vcpkg installed at `C:/vcpkg`

Notes:

- On Windows, LLVM/CMake/Ninja still rely on MSVC toolchain + Windows SDK libraries.
- If linker errors mention system libs (for example `kernel32.lib`), run builds from a VS Developer shell.

## Project Layout

```text
engine/
  include/engine/core/
    Application/
    Renderer/
  include/engine/platform/
  src/core/Application/
  src/platform/
  third_party/glad2/
sandbox/
  src/main.cpp
```

## Build

From the repository root:

```powershell
cmake --preset default
cmake --build --preset default
```

The `default` preset uses:

- Ninja
- `build/` as the binary directory
- vcpkg toolchain at `C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

## Run

```powershell
.\build\sandbox\sandbox.exe
```

## Known Caveats

- `sandbox/src/main.cpp` currently loads a sprite texture using an absolute local path.
- If that file does not exist on your machine, startup still runs but texture loading fails and no sprite is shown.
- Asset path/config plumbing is still pending.

## Architecture Rules

- Core depends only on core abstractions/data.
- Platform depends on core and external frameworks (SDL/OpenGL).
- Sandbox performs composition (wiring concrete implementations to core contracts).

## Milestones

- [x] Window + event loop
- [x] OpenGL renderer bootstrap
- [x] Primitive geometry path
- [x] Sprite draw + batching API path
- [ ] Input mapping
- [ ] ECS integration
- [ ] Asset pipeline
- [ ] Camera/animation/audio systems
- [ ] Editor/tooling
