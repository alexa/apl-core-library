cmake_minimum_required(VERSION 3.5)
project(Extensions
        VERSION
        1.0.0
        LANGUAGES
        CXX C)

option(BUILD_UNIT_TESTS "Build unit tests" ON)

add_subdirectory(alexaext)
#add_subdirectory(sample)

if (BUILD_UNIT_TESTS)
    add_subdirectory(unit)
endif()