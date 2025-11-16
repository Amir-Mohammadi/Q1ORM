cmake_minimum_required(VERSION 3.14)

project(UnitTestExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the compiled Q1ORM release (adjust if needed)
set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

# Imported shared lib (prebuilt)
add_library(Q1ORM SHARED IMPORTED)
set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")

# Find Qt (Core + Test; add Sql if you use QtSql in your context)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test Sql)

# Test sources
set(TEST_SOURCES
    main.cpp
     ../SoloExample/applicationdbcontext.cpp
    # add other test files if you have them
)

add_executable(UnitTestExample ${TEST_SOURCES}
    Q1ORMTests.h
    Q1ORMTests.cpp)

# Include path(s) for ApplicationDbContext and other project headers (adjust if needed)
target_include_directories(UnitTestExample PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."                 # if applicationdbcontext.h sits one level up
    "${Q1ORM_PATH}/include"                          # Q1ORM headers
)

# Link Qt Test and your Q1ORM prebuilt lib
target_link_libraries(UnitTestExample
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Sql
        Q1ORM
)

# Ensure the test executable is built next to the runtime DLLs (so Windows can load Q1ORM.dll)
set_target_properties(UnitTestExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy Q1ORM.dll (and any other required runtime DLLs) next to the test exe after build
if(WIN32)
    add_custom_command(TARGET UnitTestExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:UnitTestExample>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Q1ORM_PATH}/bin/Q1ORM.dll"
            "$<TARGET_FILE_DIR:UnitTestExample>"
        COMMENT "Copying Q1ORM.dll next to test executable"
    )
endif()

# Add the test to CTest so you can run `ctest` / `ctest -V`
enable_testing()
add_test(NAME Q1ORM_UnitTests
         COMMAND "$<TARGET_FILE:UnitTestExample>")

# Optional: install rules if you want to install the test
include(GNUInstallDirs)
install(TARGETS UnitTestExample
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
