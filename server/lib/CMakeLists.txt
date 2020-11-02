cmake_minimum_required(VERSION 3.5)

add_library(Watchdog src/watchdog.cpp)

target_include_directories(Watchdog PUBLIC 
                            $<INSTALL_INTERFACE:include>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
