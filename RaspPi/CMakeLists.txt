cmake_minimum_required(VERSION 3.10)
project(SmartPlugServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

if(EXISTS "/sys/firmware/devicetree/base/model")
    message(STATUS "Compiling for Raspberry Pi - enabling GPIO support")
    add_definitions(-DRASPBERRY_PI)
    find_library(WIRINGPI_LIB wiringPi)
    if(WIRINGPI_LIB)
        message(STATUS "Found wiringPi library")
    else()
        message(WARNING "wiringPi library not found - GPIO functionality will be simulated")
    endif()
else()
    message(STATUS "Not on Raspberry Pi - GPIO will be simulated")
endif()

include_directories(srcs)
include_directories(${LIBMICROHTTPD_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})

set(SOURCES
    srcs/main.cpp
    srcs/HTTPServer.cpp
    srcs/GPIOController.cpp
    srcs/ConfigManager.cpp
    srcs/Logger.cpp
    srcs/RelayController.cpp
)

add_executable(smart_plug_server ${SOURCES})

target_link_libraries(smart_plug_server
    ${LIBMICROHTTPD_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)

if(WIRINGPI_LIB)
    target_link_libraries(smart_plug_server ${WIRINGPI_LIB})
endif()

install(TARGETS smart_plug_server DESTINATION bin)
install(DIRECTORY config/ DESTINATION /etc/smart_plug)
install(FILES systemd/smart_plug.service DESTINATION /lib/systemd/system)