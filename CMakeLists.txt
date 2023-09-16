cmake_minimum_required(VERSION 3.26)
project(gba_emulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(gba_emulator main.cpp
        chip8.cpp
        chip8.h)
