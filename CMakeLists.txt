cmake_minimum_required(VERSION 3.20)
project(thesis-dev)

add_subdirectory(third_party_src)

add_executable(thesis
        src/main.cpp
        src/tiny_obj_loader.cpp
        src/glad.c)

target_link_libraries(thesis
        imgui
        libs/sdl2/bin/SDL2-static.lib
        lib/sdl2/bin/SDL2main.lib)

