add_library(engine STATIC)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_sources(engine PRIVATE
    src/core/Application/Application.cpp
    src/core/Renderer/Renderer.cpp
    third_party/glad2/src/gl.c
)

target_include_directories(engine 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad2/include
)

target_link_libraries(engine PUBLIC SDL3::SDL3 OpenGL::GL)