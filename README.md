# Q1ORM

**Src** 
- this folder is main orm library folder to share and use it in Example folder.

**Example**
- test library in this folder which use by Src.

**Docs**
- this folder is write Documents of project and library.

**Releases**
- this is the Realese of library which use in Example.
  
```
cmake --install <build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\Src>
```

**Installer**
- in this folder we configure automaticly cmakelists.txt in Example file to use it.




**Src CmakeLists.txt**

```
cmake_minimum_required(VERSION 3.14)

project(Src VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(PROJECT_NAME "Q1ORM")
set(CMAKE_Q1ORM_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Releases/Release-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(Q1ORM_HEADERS

    Q1ORM_global.h
    Q1ORM.h)

set(Q1ORM_SOURCES

    Q1ORM.cpp)

add_library(Src SHARED ${Q1ORM_HEADERS} ${Q1ORM_SOURCES})

target_link_libraries(Src PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(Src PRIVATE Q1ORM_LIBRARY)
set_target_properties(Src PROPERTIES OUTPUT_NAME "${PROJECT_NAME}" )

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

install(DIRECTORY / DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/qml"
        FILE_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        DIRECTORY_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE GROUP_WRITE
            WORLD_READ WORLD_EXECUTE
        FILES_MATCHING PATTERN "*.qml")

```


**Example project CmakeLists.txt**

```
cmake_minimum_required(VERSION 3.14)

project(InstallDatabaseExample LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

add_library(Q1ORM SHARED IMPORTED)

set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(InstallDatabaseExample
  main.cpp
)
target_link_libraries(InstallDatabaseExample Qt${QT_VERSION_MAJOR}::Core Q1ORM)

include(GNUInstallDirs)
install(TARGETS InstallDatabaseExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

```
