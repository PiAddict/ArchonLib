cmake_minimum_required(VERSION 4.3)
project(Archon)

set(CMAKE_CXX_STANDARD 20)

add_library(Archon STATIC library.cpp
        EntityIdBase.cpp
        EntityIdBase.h)
