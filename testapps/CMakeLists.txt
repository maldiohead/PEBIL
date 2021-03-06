# Project name is not mandatory, but you should use it
project(pebil_testapps C CXX Fortran)

# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

link_libraries("-lm")
include_directories("${CMAKE_SOURCE_DIR}/instcode")

# Create a variable called helloworld_SOURCES containing all .cpp files:
set(dynTest_SOURCES dynTest.c)
set(cTest_SOURCES foo.c bar.c dum.c main.c)
set(cppTest_SOURCES classes.C)
set(htcoef_SOURCES htcoef.f)
set(sgbench_SOURCES sgbench.c)

# Create an executable file called helloworld from sources:
add_executable(dynTest ${dynTest_SOURCES})
add_executable(cTest ${cTest_SOURCES})
add_executable(cppTest ${cppTest_SOURCES})
add_executable(htcoef ${htcoef_SOURCES})
add_executable(sgbench ${sgbench_SOURCES})

add_test(NAME pebilide-dynTest
  COMMAND pebil --typ ide --app dynTest)
add_test(NAME pebilide-cTest
  COMMAND pebil --typ ide --app cTest)
add_test(NAME pebilide-cppTest                                                                                                                                        
  COMMAND pebil --typ ide --app testapps/cppTest) 
add_test(NAME pebilide-htcoef
  COMMAND pebil --typ ide --app htcoef)
