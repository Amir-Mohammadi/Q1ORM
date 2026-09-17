cmake_minimum_required(VERSION 3.14)
project(Q1ORM)

include(CTest)
option(Q1ORM_BUILD_TOOLS "Build release helper tools" OFF)

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(Examples)
endif()
add_subdirectory(Docs)
if(Q1ORM_BUILD_TOOLS)
    add_subdirectory(Tools)
endif()
