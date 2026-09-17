cmake_minimum_required(VERSION 3.14)
project(TestExample LANGUAGES CXX)

if(NOT TARGET Q1ORM::Q1ORM)
    include(CTest)
    add_subdirectory(../../src ${CMAKE_CURRENT_BINARY_DIR}/q1orm)
endif()

find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(Threads REQUIRED)

add_executable(TestExample
    main.cpp
    OrmTests.cpp
    OrmTests.h
    ModelBuilderTests.cpp
    SchemaScaleTests.cpp
    SchemaScale/ApplicationDbContext.cpp
    SchemaScale/Maps00.cpp
    SchemaScale/Maps01.cpp
    SchemaScale/Maps02.cpp
    SchemaScale/Maps03.cpp
    SchemaScale/Maps04.cpp
    SchemaScale/Maps05.cpp
    SchemaScale/Maps06.cpp
    SchemaScale/Maps07.cpp
    SchemaScale/Maps08.cpp
    SchemaScale/Maps09.cpp
    SchemaScale/Maps10.cpp
    SchemaScale/Maps11.cpp
    SchemaScale/Maps12.cpp
    SchemaScale/Maps13.cpp
    SchemaScale/Maps14.cpp
)
set_target_properties(TestExample PROPERTIES
    AUTOMOC ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(TestExample PRIVATE Q1ORM::Q1ORM Qt6::Test Threads::Threads)

if(WIN32)
    add_custom_command(TARGET TestExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Src>" "$<TARGET_FILE_DIR:TestExample>"
    )
endif()

foreach(test_case IN ITEMS
    crud bulkCrud selects boundValues aggregates grouping joinsAndIncludes
    jsonAndRawSql changeTracking transactions constraints errorHandling sqlGeneration connectionLifecycle
    threadedCrud services modelBuilder schemaScale
)
    add_test(NAME Q1ORM.${test_case} COMMAND TestExample ${test_case})
    set_tests_properties(Q1ORM.${test_case} PROPERTIES
        TIMEOUT 180
        RUN_SERIAL TRUE
    )
endforeach()
set_tests_properties(Q1ORM.schemaScale PROPERTIES LABELS "sqlite;scale")
set_tests_properties(Q1ORM.modelBuilder PROPERTIES LABELS "sqlite;schema")
