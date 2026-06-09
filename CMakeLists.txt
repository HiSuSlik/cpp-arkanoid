cmake_minimum_required(VERSION 3.16)
project(ArkanoidLabSFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
)

FetchContent_MakeAvailable(sfml)

add_executable(ArkanoidLabSFML
    src/main.cpp
    src/Game.cpp
)

target_include_directories(ArkanoidLabSFML PRIVATE include)
target_link_libraries(ArkanoidLabSFML PRIVATE sfml-graphics sfml-window sfml-system sfml-main)

if (MSVC)
    target_compile_options(ArkanoidLabSFML PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(ArkanoidLabSFML PRIVATE -Wall -Wextra -Wpedantic)
endif()
