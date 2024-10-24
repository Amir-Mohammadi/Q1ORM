cmake_minimum_required(VERSION 3.14)

project(ReleaseInstaller VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(App_Icon "icon.rc")

set(PROJECT_NAME "ReleaseInstaller")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(ReleaseInstaller main.cpp ${App_Icon})
set_target_properties(ReleaseInstaller PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

target_link_libraries(ReleaseInstaller Qt${QT_VERSION_MAJOR}::Core)

include(GNUInstallDirs)
install(TARGETS ReleaseInstaller
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
