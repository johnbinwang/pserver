# set default build type to Debug 
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)")

project(pserver)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/fitz)
include_directories(${CMAKE_SOURCE_DIR}/static)
include_directories(/usr/local/include/thrift)
include_directories(/usr/local/include/ImageMagick)
include_directories(/usr/local/lib)

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/static/)

set(PSERVER_VERSION "0.7")
set(ARCHIVE_NAME pserver-${PSERVER_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(PkgConfig)

set(CMAKE_C_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

# release build flags (overwrite default cmake release flags)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# generic flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# clang compiler need c++11 flag
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# CYGWIN or GCC 4.5.x bug
if(CYGWIN)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++0x")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("${CMAKE_CXX_FLAGS}" CXX0X_SUPPORT)
if(NOT CXX0X_SUPPORT)
    message(FATAL_ERROR "Error: you compiler does not support C++0x, please update it.")
endif()


configure_file (${CMAKE_SOURCE_DIR}/pserver.1.in ${CMAKE_SOURCE_DIR}/pserver.1)

add_executable(pserver
	src/PdfConverter_types.cpp
	src/PdfConverter_types.h
	src/PdfConverter.cpp
	src/PdfConverter.h
	src/PdfConverter_constants.h
	src/PdfConverter_constants.cpp
	src/main.cpp
	src/pdf_worker.h
	src/pdf_worker.c
    src/util/path.h
    src/util/path.cc
    )
target_link_libraries(pserver thrift)
target_link_libraries(pserver thriftnb)
target_link_libraries(pserver event)
target_link_libraries(pserver m)
target_link_libraries(pserver z)
target_link_libraries(pserver png)
target_link_libraries(pserver libfitz.a)
target_link_libraries(pserver libfreetype.a)
target_link_libraries(pserver libopenjpeg.a)
target_link_libraries(pserver libjbig2dec.a)
target_link_libraries(pserver libjpeg.a)
target_link_libraries(pserver Magick++-Q16)

install (TARGETS pserver DESTINATION bin)
file (GLOB datafiles 
    )
install (FILES ${datafiles} DESTINATION share/pserver)
install (FILES pserver.1 DESTINATION share/man/man1)
