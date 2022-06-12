cmake_minimum_required(VERSION 3.20)
project(thesis-dev)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(third_party_src)
add_subdirectory(libs/focus)

add_executable(thesis
        src/main.cpp
        src/tiny_obj_loader.cpp
        )

target_include_directories(thesis PRIVATE
        #libs/KHR/include
        libs/SDL2/include
        libs/glm/
        libs/entt/include
        libs/focus/)

if (WIN32)
    set_property(TARGET thesis PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:DEBUG>:Debug>")
    target_compile_definitions(thesis PUBLIC HAVE_LIBC=1)
    target_link_libraries(thesis PUBLIC
            imgui
            focus
            ${CMAKE_SOURCE_DIR}/libs/sdl2/bin/SDL2.lib
            ${CMAKE_SOURCE_DIR}/libs/sdl2/bin/SDL2main.lib
            winmm.lib
            imm32.lib
            version.lib
            opengl32
            Setupapi.lib)
    # I would be static linking, but I ran into an issue with duplicate symbols for memcpy, which won't be fixed in SDL2
    # until the next release.
    add_custom_command(TARGET thesis POST_BUILD 
			COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${PROJECT_SOURCE_DIR}/libs/SDL2/bin/SDL2.dll" $<TARGET_FILE_DIR:thesis>)

endif ()
