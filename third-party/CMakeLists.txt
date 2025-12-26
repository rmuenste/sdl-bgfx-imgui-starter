cmake_minimum_required(VERSION 3.24)

if (NOT SUPERBUILD)
  project(third-party)
endif ()

include(ExternalProject)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT isMultiConfig)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE)
  endif ()
  set(build_type_dir ${CMAKE_BUILD_TYPE})
  set(build_type_arg -DCMAKE_BUILD_TYPE=$<CONFIG>)
else ()
  set(build_config_arg --config=$<CONFIG>)
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  set(THIRD_PARTY_CMAKE_COMMAND emcmake cmake)
else ()
  set(THIRD_PARTY_CMAKE_COMMAND ${CMAKE_COMMAND})
endif ()

if (SUPERBUILD)
  set(PREFIX_DIR
      ${CMAKE_CURRENT_SOURCE_DIR}/third-party/${THIRD_PARTY_BUILD_DIR_NAME})
else ()
  set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif ()

if (NOT EMSCRIPTEN)
  ExternalProject_Add(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.4
    PREFIX ${PREFIX_DIR}
    BINARY_DIR ${PREFIX_DIR}/src/SDL2-build/${build_type_dir}
    CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
    CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
    INSTALL_COMMAND cmake --build <BINARY_DIR> --target install
                    ${build_config_arg})
endif ()

ExternalProject_Add(
  bgfx
  GIT_REPOSITORY https://github.com/pr0g/bgfx.cmake.git
  GIT_TAG bde3f94ce75ffd184e9a3bdfe947a3bec69233eb
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/bgfx-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             "$<$<PLATFORM_ID:Emscripten>:-DBGFX_CONFIG_MULTITHREADED=OFF>"
             -DBGFX_BUILD_EXAMPLES=${BGFX_BUILD_EXAMPLES}
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install
                  ${build_config_arg})

ExternalProject_Add(
  imgui.cmake
  GIT_REPOSITORY https://github.com/pr0g/imgui.cmake.git
  GIT_TAG 20e7d1a627690526c98b0b48c346e384ab87c5a6
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/imgui.cmake-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=ON
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install
                  ${build_config_arg})

# PE (Physics Engine) - Rigid body dynamics simulation
# PE requires Boost - check that it's available
find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "PE requires Boost >= 1.46.1. Please install Boost first.")
endif()

# Check for CGAL if enabled
if (PE_BUILD_CGAL)
  find_package(CGAL QUIET)
  if (NOT CGAL_FOUND)
    message(WARNING "CGAL not found. PE will be built without CGAL support.")
    set(PE_BUILD_CGAL OFF)
  endif()
endif()

ExternalProject_Add(
  pe
  GIT_REPOSITORY https://github.com/rmuenste/pe-0.3rc1.git
  GIT_TAG master
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/pe-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             # PE Configuration
             -DLIBRARY_TYPE=STATIC
             -DEXAMPLES=OFF
             -DIRRLICHT=OFF
             -DEIGEN=ON
             -DCGAL=${PE_BUILD_CGAL}
             -DUSE_JSON=ON
             -DMPI=OFF
             -DOPENCL=OFF
             -DBLAS=OFF
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  # PE doesn't have install targets, so we manually copy files
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
                  <SOURCE_DIR>/pe
                  <INSTALL_DIR>/include/pe
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  <BINARY_DIR>/config.h
                  <INSTALL_DIR>/include/config.h
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  <BINARY_DIR>/lib/libpe.a
                  <INSTALL_DIR>/lib/libpe.a
          COMMAND ${CMAKE_COMMAND} -E echo "PE installed to <INSTALL_DIR>")
