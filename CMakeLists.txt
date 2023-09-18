cmake_minimum_required(VERSION 3.24)
project(gba_emulator)

set(CMAKE_CXX_STANDARD 17)


set(SFML_STATIC_LIBRARIES TRUE)

set(SFML_DIR "/home/jc/projects/cpp/SFML-2.6.0/lib/cmake/SFML/")

find_package(SFML 2.6.0 COMPONENTS graphics window system REQUIRED)

add_executable(gba_emulator WIN32 main.cpp
        chip8.cpp
        chip8.h)

target_link_libraries(gba_emulator sfml-graphics sfml-window sfml-system)


