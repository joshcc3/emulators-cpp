cmake_minimum_required(VERSION 3.24)

project(gba_emulator)
#project(shared_mem)
#project(leetcode)
#project(chip8)
#project(audio_test)

set(CMAKE_CXX_STANDARD 20)

set(SFML_DIR "/home/jc/projects/cpp/SFML-2.6.0/lib/cmake/SFML")
#set(SFML_DIR "/home/jc/CLionProjects/SFML")

find_package(SFML 2.6.0 REQUIRED COMPONENTS audio network graphics window system)

find_package(Boost 1.78.0 COMPONENTS program_options REQUIRED)

#add_executable(
#        shared_mem
#        boost_test.cpp
#)
#
#add_executable(
#        chip8
#        chip8/chip8.cpp
#        chip8/chip8.h
#)
#
#add_executable(
#        leetcode
#        gameboy/stacksAndQueues.cpp gameboy/stacksAndQueues.h
#)
#
#add_executable(
#        audio_test
#        gameboy/audio_test.cpp
#        gameboy/video_test.h
#)

add_executable(
        gba_emulator
        gameboy/main.cpp
        gameboy/audio_driver.cpp
        gameboy/audio_driver.h
        gameboy/debug_utils.h
        gameboy/PPU.cpp
        gameboy/PPU.h
        gameboy/structs.h
)

target_link_libraries(gba_emulator sfml-graphics sfml-window sfml-audio sfml-system asound ${BOOST_LIBRARIES})
#target_link_libraries(gba_emulator /home/jc/CLionProjects/SFML/lib/libsfml-audio-ringBufferSize.a /home/jc/CLionProjects/SFML/lib/libsfml-system-ringBufferSize.a)
#target_link_libraries(Test01 sfml-graphics sfml-audio sfml-network sfml-window sfml-system)
