cmake_minimum_required(VERSION 3.14)

project(DbExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)
find_package(Threads REQUIRED)

if(TARGET Src)
    set(Q1ORM_TARGET Src)
    set(Q1ORM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src")
else()
    set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")
    add_library(Q1ORM SHARED IMPORTED)
    set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
    set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
    target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")
    set(Q1ORM_TARGET Q1ORM)
    set(Q1ORM_INCLUDE_DIR "${Q1ORM_PATH}/include")
endif()

add_executable(DbExample main.cpp)

target_include_directories(DbExample PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${Q1ORM_INCLUDE_DIR}"
)

target_link_libraries(DbExample
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Sql
        Threads::Threads
        ${Q1ORM_TARGET}
)

set_target_properties(DbExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(WIN32)
    add_custom_command(TARGET DbExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:DbExample>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${Q1ORM_TARGET}>"
            "$<TARGET_FILE_DIR:DbExample>"
        COMMENT "Copying the Q1ORM runtime next to the example executable"
    )

    # libmysql.dll: وابستگی پلاگین qsqlmysql.dll
    set(MYSQL_LIBMYSQL "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.dll"
        CACHE FILEPATH "Path to libmysql.dll (x64)")

    if(EXISTS "${MYSQL_LIBMYSQL}")
        add_custom_command(TARGET DbExample POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MYSQL_LIBMYSQL}"
                "$<TARGET_FILE_DIR:DbExample>"
            COMMENT "Copying libmysql.dll next to the example executable"
        )
    else()
        message(WARNING "libmysql.dll not found at ${MYSQL_LIBMYSQL} — QMYSQL driver will fail to load")
    endif()
endif()

enable_testing()
add_test(NAME Q1ORM_DbExample
         COMMAND "$<TARGET_FILE:DbExample>")

add_executable(ModelBuilderTest model_builder_test.cpp)
target_link_libraries(ModelBuilderTest PRIVATE
    ${Q1ORM_TARGET} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql)
add_test(NAME Q1ORM_ModelBuilder COMMAND ModelBuilderTest)
