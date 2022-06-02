
add_library(imgui STATIC
    imgui.cpp
    imgui.h
    imgui_impl_opengl3.h
    imgui_impl_opengl3.cpp
    imgui_impl_sdl.h
    imgui_impl_sdl.cpp
    )
target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/libs/sdl2/include/sdl2)
