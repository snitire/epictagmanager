cmake_minimum_required(VERSION 3.30)
project(epictagmanager)

set(CMAKE_CXX_STANDARD 20)

add_executable(epictagmanager main.cpp
        src/tag_manager.cpp
        src/tag_manager.h
        src/file_handler.cpp
        src/file_handler.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
target_link_libraries(epictagmanager PRIVATE PkgConfig::TAGLIB)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(epictagmanager PRIVATE argparse::argparse)

install(TARGETS epictagmanager DESTINATION bin)
