cmake_minimum_required(VERSION 3.21)

project(ReleaseInstaller
    VERSION 0.1
    LANGUAGES CXX
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


set(PROJECT_NAME "ReleaseInstaller")


find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()


add_executable(${PROJECT_NAME}
    main.cpp
    icon.rc
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
)


set_target_properties(${PROJECT_NAME}
    PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
)


include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)