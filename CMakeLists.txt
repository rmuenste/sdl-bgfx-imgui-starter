cmake_minimum_required(VERSION 3.24)

option(SUPERBUILD "Perform a superbuild (or not)" OFF)
if (NOT DEFINED BGFX_BUILD_EXAMPLES)
  if (CMAKE_CONFIGURATION_TYPES)
    set(_bgfx_build_examples_default OFF)
  elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_bgfx_build_examples_default ON)
  else ()
    set(_bgfx_build_examples_default OFF)
  endif ()
  set(BGFX_BUILD_EXAMPLES
      ${_bgfx_build_examples_default}
      CACHE BOOL "Build bgfx examples in the superbuild")
  unset(_bgfx_build_examples_default)
endif ()

option(PE_BUILD_CGAL "Build PE with CGAL support for triangle meshes" ON)

project(sdl-bgfx-imgui-starter LANGUAGES CXX)

if (SUPERBUILD)
  if (EMSCRIPTEN)
    set(THIRD_PARTY_BUILD_DIR_NAME embuild)
  else ()
    set(THIRD_PARTY_BUILD_DIR_NAME build)
  endif ()
  include(third-party/CMakeLists.txt)
  include(cmake/superbuild.cmake)
  return()
endif ()

find_package(SDL2 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(bgfx REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(imgui.cmake REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)

# PE doesn't export CMake config, so we set paths manually
# The superbuild will build PE and we can link to it directly
set(PE_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)
set(PE_LIBRARY_DIR ${CMAKE_PREFIX_PATH}/lib)
find_library(PE_LIBRARY NAMES pe libpe PATHS ${PE_LIBRARY_DIR} NO_DEFAULT_PATH)

# Include helper functions for creating bgfx applications
include(cmake/common-helpers.cmake)

# Build common library (ImGui impls, debugdraw)
add_subdirectory(common)

# Build main executable using helper function
add_bgfx_application(
  NAME ${PROJECT_NAME}
  SOURCES main.cpp
)

# Build examples
add_subdirectory(examples)
