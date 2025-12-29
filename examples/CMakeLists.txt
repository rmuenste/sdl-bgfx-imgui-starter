# Examples directory
#
# This CMakeLists.txt auto-discovers all example subdirectories and builds them.
# Each example directory should contain its own CMakeLists.txt that uses the
# add_bgfx_application() helper function.
#
# To add a new example:
# 1. Create a directory: examples/XX-my-example/
# 2. Add main.cpp with your example code
# 3. Create CMakeLists.txt with:
#      add_bgfx_application(
#          NAME XX-my-example
#          SOURCES main.cpp
#      )
#
# The example will be automatically discovered and built.

# Get all subdirectories
file(GLOB EXAMPLE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

# Add each subdirectory that contains a CMakeLists.txt
foreach(EXAMPLE_DIR ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_DIR})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_DIR}/CMakeLists.txt)
            message(STATUS "Adding example: ${EXAMPLE_DIR}")
            add_subdirectory(${EXAMPLE_DIR})
        endif()
    endif()
endforeach()
