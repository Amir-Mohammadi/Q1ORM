cmake_minimum_required(VERSION 3.14)
project(AppExample LANGUAGES CXX)

if(NOT TARGET Q1ORM::Q1ORM)
    include(CTest)
    add_subdirectory(../../src ${CMAKE_CURRENT_BINARY_DIR}/q1orm)
endif()

find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(Threads REQUIRED)

add_executable(AppExample
    main.cpp
    )

set_target_properties(AppExample PROPERTIES
    AUTOMOC ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(AppExample PRIVATE Q1ORM::Q1ORM Qt6::Test Threads::Threads)

if(WIN32)
    add_custom_command(TARGET AppExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Src>" "$<TARGET_FILE_DIR:AppExample>"
    )
endif()

foreach(test_case IN ITEMS
    crud bulkCrud selects boundValues aggregates grouping joinsAndIncludes
    jsonAndRawSql changeTracking transactions constraints errorHandling sqlGeneration connectionLifecycle
    threadedCrud services modelBuilder schemaScale
)
    add_test(NAME Q1ORM.${test_case} COMMAND AppExample ${test_case})
    set_tests_properties(Q1ORM.${test_case} PROPERTIES
        TIMEOUT 180
        RUN_SERIAL TRUE
    )
endforeach()

set_tests_properties(Q1ORM.schemaScale PROPERTIES LABELS "sqlite;scale")
set_tests_properties(Q1ORM.modelBuilder PROPERTIES LABELS "sqlite;schema")
