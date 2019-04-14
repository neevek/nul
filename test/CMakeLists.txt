cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_CXX_FLAGS "-g -Wall -std=c++1z -DLOG_VERBOSE -DLOG_TAG_NAME='\"nul\"'")
set(NUL_SRC_DIR ${CMAKE_SOURCE_DIR}/../src)
include_directories(${NUL_SRC_DIR})

find_package(Threads REQUIRED)


# macro to set library properties
macro(SET_LIBRARY_PROP LIB_NAME DEPENDENCY LIB_FILE)
  # Create a libgmock target to be used as a dependency by test programs
  add_library(${LIB_NAME} IMPORTED STATIC GLOBAL)
  add_dependencies(${LIB_NAME} ${DEPENDENCY})

  # Set libgmock properties
  set_target_properties(${LIB_NAME} PROPERTIES
    "IMPORTED_LOCATION" ${LIB_FILE}
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    )
endmacro()


# Enable ExternalProject CMake module
include(ExternalProject)

# add GoogleTest
ExternalProject_Add(gtest
  PREFIX "${CMAKE_BINARY_DIR}/deps"
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "master"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/deps/installed
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  UPDATE_DISCONNECTED 1
)
# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest INSTALL_DIR)
set(GTEST_INCLUDE_DIR ${INSTALL_DIR}/installed/include)
set(GTEST_LIBRARY_DIR ${INSTALL_DIR}/installed/lib)
SET_LIBRARY_PROP(libgtest gtest "${GTEST_LIBRARY_DIR}/libgtest.a")
SET_LIBRARY_PROP(libgmock gtest "${GTEST_LIBRARY_DIR}/libgmock.a")

include_directories(
  "${GTEST_INCLUDE_DIR}"
  ) 

set(COMMON_SRCS main.cc)
set(COMMON_LINK_LIBS libgtest libgmock)

macro(ADD_NUL_TEST TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${COMMON_SRCS} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} ${COMMON_LINK_LIBS})
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

ADD_NUL_TEST(xbuffer nul/xbuffer.cc)
ADD_NUL_TEST(util nul/util.cc)
ADD_NUL_TEST(uri nul/uri.cc)
ADD_NUL_TEST(circular_buffer nul/circular_buffer.cc)
