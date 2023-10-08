cmake_minimum_required(VERSION 3.24)
include_directories(/usr/include)

project(gba_emulator)

set(CMAKE_CXX_STANDARD 17)

set(SFML_STATIC_LIBRARIES TRUE)

set(SFML_DIR "/home/jc/projects/cpp/SFML-2.6.0/lib/cmake/SFML/")
#set(SFML_DIR "/home/jc/CLionProjects/SFML")

find_package(SFML 2.6.0 COMPONENTS graphics window system audio REQUIRED)
Find_Boost()

add_executable(gba_emulator #main.cpp
        chip8.cpp
        chip8.h gameboy/gameboy.cpp gameboy/gameboy.h gameboy/gb_audio.cpp gameboy/gb_audio.h
        #gameboy/audio_test.cpp gameboy/audio_test.h
        #gameboy/video_test.cpp
        gameboy/video_test.h
        gameboy/audio_driver.cpp gameboy/audio_driver.h
        gameboy/debug_utils.h gameboy/stacksAndQueues.cpp gameboy/stacksAndQueues.h)

target_link_libraries(gba_emulator sfml-graphics sfml-window sfml-audio sfml-system asound ${BOOST_LIBRARIES})
#target_link_libraries(gba_emulator /home/jc/CLionProjects/SFML/lib/libsfml-audio-ringBufferSize.a /home/jc/CLionProjects/SFML/lib/libsfml-system-ringBufferSize.a)


