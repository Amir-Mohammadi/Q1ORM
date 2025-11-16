cmake_minimum_required(VERSION 3.14)

project(Src VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

set(PROJECT_NAME "Q1ORM")
set(CMAKE_Q1ORM_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Releases/Release-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(Q1ORM_SCRIPTS
    Q1DatabaseInstall/postgresql_install.bat)

set(Q1ORM_HEADERS
    Q1ORM_global.h
    Q1ORM.h
    Q1DatabaseInstall/Q1DatabaseInstall.h
    Q1Core/Q1Context/Q1Context.h
    Q1Core/Q1Context/Q1Connection.h

    Q1Core/Q1Entity/Q1Entity.h
    Q1Core/Q1Entity/Q1Table.h
    Q1Core/Q1Migration/Q1MigrationQuery.h
    Q1Core/Q1Migration/Q1Migration.h
    Q1Core/Q1Entity/Q1Relation.h
)

set(Q1ORM_SOURCES
    Q1ORM.cpp
    Q1DatabaseInstall/Q1DatabaseInstall.cpp
    Q1Core/Q1Context/Q1Context.cpp
    Q1Core/Q1Entity/Q1Entity.cpp
    Q1Core/Q1Migration/Q1MigrationQuery.cpp
    Q1Core/Q1Migration/Q1Migration.cpp
)

add_library(Src SHARED ${Q1ORM_HEADERS} ${Q1ORM_SOURCES} ${Q1ORM_SCRIPTS}
    Q1Core/Q1Query/Q1Query.h
    Q1Core/Q1Entity/Q1Column.h
    Q1Core/Q1Migration/Q1PgMigrationQuery.h Q1Core/Q1Migration/Q1PgMigrationQuery.cpp
    Q1Core/Q1Migration/Q1SQLServerMigrationQuery.h Q1Core/Q1Migration/Q1SQLServerMigrationQuery.cpp
    Q1Core/Q1Migration/Q1SqlServerMigrationQuery.h Q1Core/Q1Migration/Q1SqlServerMigrationQuery.cpp
    Q1Core/Q1Migration/Q1SqlServerMigrationQuery.cpp Q1Core/Q1Migration/Q1SqlServerMigrationQuery.h
    Q1Core/Q1Entity/Q1Column.cpp

)

target_link_libraries(Src PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql)

target_compile_definitions(Src PRIVATE Q1ORM_LIBRARY)
set_target_properties(Src PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")


install(TARGETS Src
        RUNTIME DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/bin"
        LIBRARY DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/lib"
        ARCHIVE DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/lib")

install(DIRECTORY / DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/include"
        FILE_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        DIRECTORY_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE GROUP_WRITE
            WORLD_READ WORLD_EXECUTE
        FILES_MATCHING PATTERN "*.h")

install(DIRECTORY Q1DatabaseInstall DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/scripts"
        FILE_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        DIRECTORY_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE GROUP_WRITE
            WORLD_READ WORLD_EXECUTE
        FILES_MATCHING PATTERN "*.bat")
