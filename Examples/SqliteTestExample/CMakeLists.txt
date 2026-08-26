cmake_minimum_required(VERSION 3.14)

project(SqliteTestExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

if(TARGET Src)
    set(Q1ORM_TARGET Src)
else()
    set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

    add_library(Q1ORM SHARED IMPORTED)
    set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
    set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
    target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")

set(Q1ORM_TARGET Q1ORM)
endif()

add_executable(SqliteTestExample
    main.cpp
    applicationdbcontext.cpp
    applicationdbcontext.h
    Models/Country.h
    Models/City.h
    Models/Club.h
    Models/Court.h
    Models/User.h
    Mapping/CountryMap.h
    Mapping/CityMap.h
    Mapping/ClubMap.h
    Mapping/CourtMap.h
    Mapping/UserMap.h
    Repository/IRepository.h
    Repository/Repository.h
    Services/InsertService.h
)


target_link_libraries(SqliteTestExample Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql ${Q1ORM_TARGET})

include(GNUInstallDirs)
install(TARGETS SqliteTestExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
