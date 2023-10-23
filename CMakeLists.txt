cmake_minimum_required(VERSION 3.24)

project(gba_emulator)
project(shared_mem1)
project(shared_mem2)
project(leetcode)
project(chip8)
project(audio_test)
project(vram_viz)
project(debug_playground)
project(multithread_investigation)


set(CMAKE_CXX_STANDARD 20)

set(SFML_DIR "/home/jc/projects/cpp/SFML-2.6.0/lib/cmake/SFML")
#set(SFML_DIR "/home/jc/CLionProjects/SFML")

find_package(SFML 2.6.0 REQUIRED COMPONENTS audio network graphics window system)

find_package(Boost 1.78.0 COMPONENTS program_options REQUIRED)

add_executable(
        debug_playground
        debug_code.cpp
        debug_code.h
)

add_executable(
        audio_test
        gameboy/audio_driver2.cpp
)

add_executable(
        multithread_investigation
        multithreading_investigation.cpp
)

add_executable(
        shared_mem1
        boost_test.cpp
)
add_executable(
        shared_mem2
        boost_test2.cpp
)

add_executable(
        chip8
        chip8/chip8.cpp
        chip8/chip8.h
)

add_executable(
        leetcode
        gameboy/l3orderbook.cpp gameboy/l3orderbook.h
)

add_executable(
        gba_emulator
        gameboy/main.cpp
        gameboy/audio_driver.h
        gameboy/debug_utils.h
        gameboy/PPU.h
        gameboy/structs.h
        gameboy/ShmAlloc.h
        gameboy/Memory.cpp
        gameboy/Memory.h
)



add_executable(vram_viz gameboy/vram_viz.cpp)



target_link_libraries(gba_emulator sfml-graphics sfml-window sfml-audio sfml-system asound ${BOOST_LIBRARIES})
target_link_libraries(vram_viz sfml-graphics sfml-window sfml-audio sfml-system asound ${BOOST_LIBRARIES})
target_link_libraries(audio_test asound)
