cmake_minimum_required(VERSION 3.16)
project(libdji VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth Test)

qt_add_library(dji STATIC
    include/dji/message.h
    include/dji/device.h
    include/dji/subsystem_pairer.h
    include/dji/subsystem_streamer.h
    include/dji/subsystem_configurer.h
    include/dji/crc.h
    src/message.cpp
    src/device.cpp
    src/subsystem_pairer.cpp
    src/subsystem_streamer.cpp
    src/subsystem_configurer.cpp
    src/crc.cpp
)

target_include_directories(dji PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(dji PUBLIC
    Qt6::Core
    Qt6::Bluetooth
)

set_target_properties(dji PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
