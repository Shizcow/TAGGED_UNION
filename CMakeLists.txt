cmake_minimum_required(VERSION 3.14)
project(tagged_union LANGUAGES CXX)

option(BUILD_TESTING "Build tests with CTest" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

# Dependency: Boost
find_package(Boost REQUIRED)

# Dependency: Magic Enum
ExternalProject_Add(
    magic_enum
    PREFIX "vendor/magic_enum"
    GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
    GIT_TAG v0.9.3
    TIMEOUT 10
    CMAKE_ARGS
        -DMAGIC_ENUM_OPT_BUILD_EXAMPLES=OFF
        -DMAGIC_ENUM_OPT_BUILD_TESTS=OFF
        -DMAGIC_ENUM_OPT_INSTALL=OFF
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(magic_enum source_dir)
set(MAGIC_ENUM_INCLUDE_DIR ${source_dir}/include)
include_directories(${MAGIC_ENUM_INCLUDE_DIR})

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
