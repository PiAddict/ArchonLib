cmake_minimum_required(VERSION 3.20)
project(Archon LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(ARCHON_BUILD_TESTING_DEFAULT ON)
else()
    set(ARCHON_BUILD_TESTING_DEFAULT OFF)
endif()
option(ARCHON_BUILD_TESTING "Build Archon's test suite" ${ARCHON_BUILD_TESTING_DEFAULT})

add_library(Archon STATIC
        src/Archon/ComponentRegistry.cpp
        src/Archon/ComponentMask.cpp
        src/Archon/EntityId.cpp
        src/Archon/EntityView.cpp
        src/Archon/EntityManager.cpp
        src/Archon/ArchetypeStorage.cpp
        src/Archon/ArchetypeRegistry.cpp
        )

target_compile_features(Archon PUBLIC cxx_std_20)
set_target_properties(Archon PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(Archon
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(Archon::Archon ALIAS Archon)

if(ARCHON_BUILD_TESTING)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.18.0
            GIT_SHALLOW TRUE)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
