cmake_minimum_required(VERSION 3.20)
project(Archon LANGUAGES CXX)

add_library(Archon STATIC
        src/Archon/Component.cpp
        src/Archon/ComponentRegistry.cpp
        src/Archon/ComponentMask.cpp
        src/Archon/EntityId.cpp
        src/Archon/EntityManager.cpp
        src/Archon/ArchetypeStorage.cpp
        src/Archon/ArchetypeRegistry.cpp
        )

target_compile_features(Archon PUBLIC cxx_std_20)

target_include_directories(Archon
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(Archon::Archon ALIAS Archon)
