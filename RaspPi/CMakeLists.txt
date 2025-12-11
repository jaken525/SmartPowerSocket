cmake_minimum_required(VERSION 3.10)
project(SmartPlugServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(WIRINGPI REQUIRED wiringPi)

find_path(I2C_INCLUDE_DIR NAMES i2c/smbus.h)
find_library(I2C_LIBRARY NAMES i2c)

if(I2C_INCLUDE_DIR AND I2C_LIBRARY)
    message(STATUS "Found I2C library")
    set(HAVE_I2C TRUE)
else()
    message(WARNING "I2C library not found - I2C sensors will be simulated")
    set(HAVE_I2C FALSE)
endif()

include_directories(src ${LIBMICROHTTPD_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} ${I2C_INCLUDE_DIR})

set(SOURCES
    srcs/main.cpp
    srcs/HTTPServer.cpp
    srcs/GPIOController.cpp
    srcs/ConfigManager.cpp
    srcs/Logger.cpp
    srcs/RelayController.cpp
    srcs/PowerMonitor.cpp
    srcs/SensorManager.cpp
    srcs/Statistics.cpp
)

add_executable(smart_plug_server ${SOURCES})

target_link_libraries(smart_plug_server
    ${LIBMICROHTTPD_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${WIRINGPI_LIBRARIES}
    pthread
)

if(HAVE_I2C)
    target_link_libraries(smart_plug_server ${I2C_LIBRARY})
    target_compile_definitions(smart_plug_server PRIVATE HAVE_I2C=1)
endif()

# Установка
install(TARGETS smart_plug_server DESTINATION /usr/local/bin)
install(DIRECTORY config/ DESTINATION /etc/smart_plug)
install(FILES systemd/smart_plug.service DESTINATION /lib/systemd/system)