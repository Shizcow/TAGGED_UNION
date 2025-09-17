cmake_minimum_required(VERSION 3.14)
project(tagged_union LANGUAGES CXX)

option(BUILD_TESTING "Build tests with CTest" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

# Dependency: Boost
find_package(Boost REQUIRED)

# Library
add_library(tagged_union INTERFACE)
target_include_directories(tagged_union INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tagged_union INTERFACE Boost::headers)

# Enable testing globally
if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
