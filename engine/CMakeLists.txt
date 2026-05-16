add_library(engine STATIC)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_sources(engine PRIVATE
    src/core/Application/Application.cpp
    src/core/Renderer/OpenGLRenderer.cpp
    src/platform/SdlWindowSurface.cpp
    third_party/glad2/src/gl.c
)

target_include_directories(engine 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad2/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine PUBLIC SDL3::SDL3 OpenGL::GL)