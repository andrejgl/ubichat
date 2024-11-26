cmake_minimum_required(VERSION 3.16)

project(UbiLib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(UbiLib STATIC

  unixsignalhandler.cpp
)

target_compile_definitions(UbiLib PRIVATE UBILIB_LIBRARY)

target_link_libraries(UbiLib
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(UbiLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

