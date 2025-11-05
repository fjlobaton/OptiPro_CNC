cmake_minimum_required(VERSION 3.31)
project(OptiPro_CNC)
set(CMAKE_CXX_STANDARD 17)

#find dependencies
find_package(SDL2 REQUIRED) #check if sdl2 is installed
find_package(OpenGL REQUIRED)  #check if openGL is installed
include(FetchContent)

#get imgui from github
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.4
)
#add imgui library files and backends
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp

        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
#include the dirs for linking
target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)
#link sdl and opengl into imgui
target_link_libraries(imgui PUBLIC
        SDL2::SDL2
        OpenGL::GL
)


add_executable(${PROJECT_NAME}
        main.cpp
)
#now link everything with main project
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui
        SDL2::SDL2
        OpenGL::GL
)