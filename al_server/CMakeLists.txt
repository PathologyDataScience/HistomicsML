cmake_minimum_required(VERSION 2.8)
project(al_server)

if( CMAKE_COMPILER_IS_GNUCC ) 
	message(STATUS "Checking compiler version...")
	EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

	if( GCC_VERSION VERSION_LESS "4.8" )
		message(FATAL_ERROR "gcc version 4.8 or greater required")
	else( GCC_VERSION VERSION_LESS "4.8" )
		message(STATUS "Compiler Ok") 
	endif( GCC_VERSION VERSION_LESS "4.8" )
endif( CMAKE_COMPILER_IS_GNUCC )


set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
# set(CMAKE_VERBOSE_MAKEFILE 1)


if( CMAKE_BUILD_TYPE STREQUAL Debug ) 
	set(AL_SERVER_DEBUG 1)
    set(CMAKE_DEBUG_POSTFIX "_d")
else( CMAKE_BUILD_TYPE STREQUAL Debug)
	set(AL_SERVER_DEBUG 0)
    set(CMAKE_DEBUG_POSTFIX "")
endif( CMAKE_BUILD_TYPE STREQUAL Debug )


add_subdirectory(src)
