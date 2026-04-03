cmake_minimum_required(VERSION 3.16)
project(tdi_reader VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

include_directories(include)

file(GLOB_RECURSE TDI_READER_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(tdi_reader ${TDI_READER_SOURCES})

if(UNIX AND NOT APPLE)
    target_compile_definitions(tdi_reader PRIVATE TDI_READER_LINUX=1)
endif()
