# NeoLab2D Engine Backend

A lightweight 2D engine backend in modern C++, currently focused on SDL windowing, application lifecycle, and an OpenGL rendering foundation.

## Current Status

Implemented so far:
- SDL3-based application initialization and window creation
- Application lifecycle management with run loop and event polling
- OpenGL renderer with context management and GLAD2 loading
- Flexible primitive rendering system (Points, Lines, Triangles, etc.)
- Vertex Array Object (VAO) and Vertex Buffer Object (VBO) management
- Shader compile/link pipeline for test geometry
- Interface-based renderer abstraction (IRenderer, IWindowSurface)
- Sandbox executable linked against engine static library
- CMake + vcpkg-based dependency setup

## Tech Stack

- Language: C++26
- Build system: CMake + Ninja
- Package manager: vcpkg (manifest mode)
- Windowing/input: SDL3
- Graphics: OpenGL 3.3 core + GLAD2

## Project Layout

- engine/: core engine static library
  - CMakeLists.txt: Engine build entry
  - include/engine/
    - core/
      - Application/: Application lifecycle management
        - Application.h
      - Renderer/: Renderer abstractions and types
        - IRenderer.h
        - OpenGLRenderer.h
        - Types.h: Primitive types and utilities
    - platform/: Platform-specific implementations
      - IWindowSurface.h
      - SdlWindowSurface.h
  - src/
    - core/
      - Application/:
        - Application.cpp
      - Renderer/:
        - OpenGLRenderer.cpp
    - platform/: Platform implementations
      - SdlWindowSurface.cpp
  - third_party/glad2/: GLAD2 OpenGL loader
- sandbox/: test executable that boots the engine
  - CMakeLists.txt: Sandbox Bootstrap build entry
  - src/main.cpp: Sandbox Bootstrap
- CMakeLists.txt: root build entry
- CMakePresets.json: default configure/build preset
- vcpkg.json: dependency manifest

## Prerequisites

Install these tools:
- CMake 3.20+
- Ninja
- Clang (clang++)
- vcpkg at C:/vcpkg

The default preset uses:
- Generator: Ninja
- Compiler: clang++
- Toolchain: C:/vcpkg/scripts/buildsystems/vcpkg.cmake

## Build

From the repository root:

```powershell
cmake --preset default
cmake --build --preset default
```

## Run

```powershell
.\build\sandbox\sandbox.exe
```

## Development Roadmap

Recommended build order:

1. ✓ Window + fixed timestep loop
2. ✓ OpenGL renderer setup
3. ✓ Primitive rendering foundation (VAO/VBO, flexible geometry types)
4. Sprite rendering and batching
5. Input action mapping
6. ECS integration (EnTT)
7. Asset manager (load/cache/lifetime)
8. Physics integration (Box2D)
9. Animation system
10. Camera system
11. Audio system (miniaudio)
12. Scene management
13. Tilemap support
14. Lua scripting (sol3)
15. Editor UI (Dear ImGui)
16. Save/load pipeline
17. Debug and profiling tools

## Architecture

### Class Diagram

```mermaid
classDiagram
    class Application {
        - m_window: SDL_Window*
        - m_renderer: unique_ptr~IRenderer~
        - m_running: bool
        - m_sdlInitialized: bool
        + Application(renderer: unique_ptr~IRenderer~)
        + ~Application()
        + init() void
        + run() void
        - cleanUp() void
    }

    class IRenderer {
        <<interface>>
        + ~IRenderer()* virtual
        + configureContextAttributes()* void
        + init(surface: IWindowSurface&)* bool
        + beginFrame()* void
        + endFrame()* void
        + resize(width: int, height: int)* void
        + shutdown()* void
        + drawTestGeometry(vertices: float*, arraySize: int, vertexCount: int, primitiveType: PrimitiveType)* void
    }

    class OpenGLRenderer {
        - m_window: SDL_Window*
        - m_context: SDL_GLContext
        - m_program: GLuint
        - m_vao: GLuint
        - m_vbo: GLuint
        - m_initialized: bool
        + ~OpenGLRenderer() override
        + configureContextAttributes() override void
        + init(surface: IWindowSurface&) override bool
        + beginFrame() override void
        + endFrame() override void
        + resize(width: int, height: int) override void
        + shutdown() override void
        + drawTestGeometry(vertices: float*, arraySize: int, vertexCount: int, primitiveType: PrimitiveType) override void
        - compileShader() bool
    }

    class PrimitiveType {
        <<enumeration>>
        Points
        Lines
        LineStrip
        LineLoop
        Triangles
        TriangleStrip
        TriangleFan
    }

    class IWindowSurface {
        <<interface>>
        + ~IWindowSurface()* virtual
        + getWindowHandle()* SDL_Window*
        + getSize(width: int&, height: int&)* void
    }

    class SdlWindowSurface {
        - m_window: SDL_Window*
        + SdlWindowSurface(window: SDL_Window*)
        + getWindowHandle() override SDL_Window*
        + getSize(width: int&, height: int&) override void
    }

    Application --> IRenderer : uses
    OpenGLRenderer --|> IRenderer : implements
    IWindowSurface <|-- SdlWindowSurface : implements
    OpenGLRenderer --> IWindowSurface : uses
    OpenGLRenderer --> PrimitiveType : uses
```

## Notes

- Build artifacts are intentionally ignored through .gitignore.
- CMake-generated files are local-environment outputs and should not be committed.

## License

No license has been set yet.
