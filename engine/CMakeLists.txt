add_library(engine STATIC)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_sources(engine PRIVATE
    src/core/Application/Application.cpp
    src/platform/SdlAppHost.cpp
    src/platform/SpriteRenderer.cpp
    src/platform/StbImageImpl.cpp
    third_party/glad2/src/gl.c
)

target_include_directories(engine 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad2/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine PRIVATE SDL3::SDL3 OpenGL::GL)