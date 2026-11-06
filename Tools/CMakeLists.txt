cmake_minimum_required(VERSION 3.14)
project(Tools)

subdirs(ExampleProjectModifier)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/Q1ReleaseRuntime.cmake")
endif()
