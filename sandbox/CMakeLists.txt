add_executable(sandbox src/main.cpp)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(sandbox PRIVATE engine SDL3::SDL3 OpenGL::GL)