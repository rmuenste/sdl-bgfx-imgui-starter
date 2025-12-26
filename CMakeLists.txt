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

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp sdl-imgui/imgui_impl_sdl2.cpp
                                       bgfx-imgui/imgui_impl_bgfx.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)

# Add PE include directory
if (PE_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} PRIVATE ${PE_INCLUDE_DIR})
endif()

target_link_libraries(
  ${PROJECT_NAME} PRIVATE SDL2::SDL2-static SDL2::SDL2main bgfx::bgfx bgfx::bx
                          imgui.cmake::imgui.cmake)

# Link PE library if found
if (PE_LIBRARY)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PE_LIBRARY})
  # PE also needs Boost
  find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
  target_link_libraries(${PROJECT_NAME} PRIVATE Boost::thread Boost::system Boost::filesystem Boost::program_options)
endif()
target_link_options(
  ${PROJECT_NAME} PRIVATE $<$<BOOL:${EMSCRIPTEN}>:-sMAX_WEBGL_VERSION=2
  -sALLOW_MEMORY_GROWTH=1 --preload-file=shader/embuild/v_simple.bin
  --preload-file=shader/embuild/f_simple.bin>)
target_compile_definitions(
  ${PROJECT_NAME} PRIVATE $<$<BOOL:${EMSCRIPTEN}>:USE_SDL=2>)

if (EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
  add_custom_command(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader/embuild
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/shader/embuild/v_simple.bin
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader/embuild
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/shader/embuild/f_simple.bin
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader/embuild
    VERBATIM)
else ()
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shader/build
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader/build
    VERBATIM)
endif ()

set_target_properties(
  ${PROJECT_NAME}
  # required for project when using visual studio generator
  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

if (WIN32)
  # copy the SDL2.dll to the same folder as the executable
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
endif ()
