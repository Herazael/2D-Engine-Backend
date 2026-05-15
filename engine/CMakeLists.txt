add_library(engine STATIC)

find_package(SDL3 CONFIG REQUIRED)

target_sources(engine PRIVATE
    src/core/Application.cpp
)

target_include_directories(engine 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine PUBLIC SDL3::SDL3)