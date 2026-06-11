cmake_minimum_required(VERSION 3.16)

project(SaperSFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SaperSFML
    main.cpp
    SaperGame.cpp
    SaperGame.h
)

find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)
target_link_libraries(SaperSFML PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)
