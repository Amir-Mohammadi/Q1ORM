include_guard(GLOBAL)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

set(Q1ORM_RELEASE_DIR "${CMAKE_SOURCE_DIR}/Releases/Release-0.1" CACHE PATH
    "Release directory populated by Q1ORMReleaseRuntime")
set(Q1ORM_RELEASE_SQL_DRIVERS "QMYSQL;QSQLITE" CACHE STRING
    "SQL drivers required in the release (QMYSQL;QSQLITE;QPSQL;QODBC)")

add_executable(Q1SqlDriverCheck "${CMAKE_CURRENT_LIST_DIR}/../Tools/SqlDriverCheck.cpp")
target_link_libraries(Q1SqlDriverCheck PRIVATE Qt6::Core Qt6::Sql)
set_target_properties(Q1SqlDriverCheck PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN/../lib")

include("${CMAKE_CURRENT_LIST_DIR}/Q1MySqlRuntime.cmake")
q1orm_deploy_mysql_runtime(Q1SqlDriverCheck)

get_target_property(q1orm_qmake Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${q1orm_qmake}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE q1orm_qt_plugins OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE q1orm_qmake_status)
if(NOT q1orm_qmake_status EQUAL 0)
    message(FATAL_ERROR "Cannot locate Qt plugins using ${q1orm_qmake}")
endif()
get_filename_component(q1orm_qt_bin "${q1orm_qmake}" DIRECTORY)
get_filename_component(q1orm_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)

configure_file("${CMAKE_CURRENT_LIST_DIR}/Q1ReleaseRuntimeDeploy.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Q1ReleaseRuntimeDeploy.cmake.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/q1orm-deploy-$<CONFIG>.cmake"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/Q1ReleaseRuntimeDeploy.cmake.in")
add_custom_target(Q1ORMReleaseRuntime
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/q1orm-deploy-$<CONFIG>.cmake"
    DEPENDS Q1SqlDriverCheck Src
    COMMENT "Packaging and checking Qt SQL runtime in ${Q1ORM_RELEASE_DIR}"
    VERBATIM)
