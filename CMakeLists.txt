cmake_minimum_required(VERSION 3.20)
project(thesis-dev)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(third_party_src)

add_executable(thesis
        src/main.cpp
        src/tiny_obj_loader.cpp
        src/glad.c)

target_include_directories(thesis PRIVATE
        libs/KHR/include
        libs/SDL2/include
        libs/glm/)

if (WIN32)
    target_link_libraries(thesis PRIVATE
            imgui
            ${CMAKE_SOURCE_DIR}/libs/sdl2/bin/SDL2-static.lib
            ${CMAKE_SOURCE_DIR}/libs/sdl2/bin/SDL2main.lib
            winmm.lib
            imm32.lib
            version.lib
            opengl32
            Setupapi.lib)

endif ()
