# Q1ORM


A lightweight **Object–Relational Mapping (ORM)** framework built with **Qt** and **C++**, designed to make database operations simple, type-safe, and cross-platform.
- This release of Q1ORM **currently supports PostgreSQL only**.  
- Support for additional databases (SQLite, SQL Server, etc.) will be added in future releases
---

## 📁 Project Structure

| Folder | Description |
|:-------|:-------------|
| **Src** | Main ORM library source code (shared between projects). |
| **Example** | Example project demonstrating how to use Q1ORM. |
| **Docs** | Documentation for classes, functions, and usage guides. |
| **Releases** | Prebuilt releases of the library used by Example projects. |
| **Installer** | Auto-configures `CMakeLists.txt` for Example to include Q1ORM. |

---

## ⚙️ Building the Project

You can build and install the Q1ORM library by running the following CMake command:

```bash
cmake --install <build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release/Src>
```

### After building, the output will be placed under:
```tree
Releases/Release-<version>
  ├── bin/
  ├── lib/
  ├── include/
  └── qml/

```

## 🧱 Src/CMakeLists.txt

```bash
# Main ORM library build configuration
# (Work in progress – subject to changes)

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
set_target_properties(Src PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

install(TARGETS Src
    RUNTIME DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/bin"
    LIBRARY DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/lib"
    ARCHIVE DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/lib")

install(DIRECTORY / DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/include"
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY / DESTINATION "${CMAKE_Q1ORM_RELEASE_DIR}/qml"
    FILES_MATCHING PATTERN "*.qml")

```
## 🧩 Example Project (InstallDatabaseExample)

```bash
# Example app linking Q1ORM library
# (Work in progress)

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

add_executable(InstallDatabaseExample main.cpp)
target_link_libraries(InstallDatabaseExample Qt${QT_VERSION_MAJOR}::Core Q1ORM)

include(GNUInstallDirs)
install(TARGETS InstallDatabaseExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

```
## 💡 Example Usage (Preview)

```cpp
// ⚠️ Work in progress — API subject to change
#include <QCoreApplication>
#include <Q1ORM.h>
#include "User.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection conn(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123", 5432);
    Q1Context context(&conn);

    User user(&context);
    user.setName("Amir");
    user.setAge(26);
    user.Save();

    auto results = user.Select().Where("age > 20").ToList(); //for return
    auto results = user.Select().Where("age > 20").ShowList(); // for show List
    auto results = user.Select().Where("age > 20").ToJson(); // for return json
    auto results = user.Select().Where("age > 20").ShowJson(); // for show json 
    return a.exec();
}

```


## 📋 Notes

### This project is under active development.

Some APIs, class names, and structure may change before v1.0 release.

Full documentation and migration tools are coming soon.

## 👨‍💻 Author

Amir Reza Mohammadi
📧 amir.mhmdi2019@gmail.com

🔗 LinkedIn :  https://www.linkedin.com/in/amir-reza-mohammadi-574160319/
