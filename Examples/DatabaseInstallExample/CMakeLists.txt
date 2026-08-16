cmake_minimum_required(VERSION 3.14)

project(DatabaseInstallExample LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(TARGET Src)
    set(Q1ORM_TARGET Src)
else()
    set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

    add_library(Q1ORM SHARED IMPORTED)
    set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
    set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
    target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")
    set(Q1ORM_TARGET Q1ORM)
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

add_executable(DatabaseInstallExample
  main.cpp
)


target_link_libraries(DatabaseInstallExample
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    ${Q1ORM_TARGET})


include(GNUInstallDirs)
install(TARGETS DatabaseInstallExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
