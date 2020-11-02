
cmake_minimum_required(VERSION 3.7)

# Listing the version is nice here since it sets lots of useful variables
project(QueueTubeService VERSION 1.0) 

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  # Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here

  # Let's ensure -std=c++xx instead of -std=g++xx
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Let's nicely support folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

	include(CheckIncludeFile)
  CHECK_INCLUDE_FILE(/usr/include/arm-linux-gnueabihf/jconfig.h RPI)
	if(${RPI})
	    	add_definitions(-DRPI_TARGET)
	else()
	    	add_definitions(-DLINUX_TARGET)
	endif()

	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	
else()

    MESSAGE(FATAL_ERROR "Your platform is currently unsupported.")

endif()

add_subdirectory(lib)
add_subdirectory(apps)


if (NOT RPI)
    SET (LIB_IDN idn)
endif()
