cmake_minimum_required(VERSION 3.2.0)
project(WavingZ)
include(CheckCXXCompilerFlag)

## Set options
set(CMAKE_CXX_STANDARD 11)

check_cxx_compiler_flag("-Wall" HAS_WALL)
if(HAS_WALL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

## Find packages
find_package(Boost 1.36 COMPONENTS program_options unit_test_framework REQUIRED)


## Targets
add_library(wavingz wavingz.cpp)
add_executable(wave-out wave-out.cpp wavingz.cpp)
add_executable(wave-in wave-in.cpp wavingz.cpp)

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(wave-in ${Boost_PROGRAM_OPTIONS_LIBRARIES} wavingz)
target_link_libraries(wave-out ${Boost_PROGRAM_OPTIONS_LIBRARIES} wavingz)

## Tests
enable_testing()
add_subdirectory(test)
