set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(FindLibMHD)
include(FindLibMPDClient)

find_package(LibMHD REQUIRED)
find_package(LibMPDClient REQUIRED)
find_package (Threads REQUIRED)

if (LIBMPDCLIENT_FOUND AND MHD_FOUND AND CMAKE_USE_PTHREADS_INIT)
  set(LIBMPDCLIENT_LIBRARIES ${LIBMPDCLIENT_LIBRARIES} "-ldl")

  add_executable(queueservice src/helpers.cpp 
                            src/queuetube_daemon.cpp 
                            src/main.cpp)

  target_link_libraries(queueservice PRIVATE Watchdog pthread ${LIBMPDCLIENT_LIBRARIES} ${MHD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
  target_include_directories(queueservice PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> ${LIBMPDCLIENT_INCLUDE_DIRS} ${MHD_INCLUDE_DIRS})
else()
	message(FATAL_ERROR "Some library is missing")
endif ()


