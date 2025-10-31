



cmake_minimum_required(VERSION 3.14)

project(JoinTablesExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

# Import your ORM DLL
add_library(Q1ORM SHARED IMPORTED)
set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")

# Find Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

# Create executable
add_executable(JoinTablesExample main.cpp Person.h
    PersonDto.h
)

# Link libraries
target_link_libraries(JoinTablesExample
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    Q1ORM
)

include(GNUInstallDirs)
install(TARGETS JoinTablesExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
