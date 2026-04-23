cmake_minimum_required(VERSION 3.16)
project(UniversalVideoDownloader VERSION 2.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(UniversalVideoDownloader
    main.cpp
    mainwindow.cpp
    mainwindow.h
    platformutils.cpp
    platformutils.h
    toolmanager.cpp
    toolmanager.h
)

target_link_libraries(UniversalVideoDownloader PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

if(APPLE)
    set_target_properties(UniversalVideoDownloader PROPERTIES MACOSX_BUNDLE TRUE)
endif()
