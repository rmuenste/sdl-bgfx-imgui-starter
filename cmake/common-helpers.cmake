# Helper function to create bgfx applications with all common dependencies and setup
#
# Usage:
#   add_bgfx_application(
#       NAME my_app
#       SOURCES main.cpp other.cpp
#   )
#
# This function handles:
# - Creating executable target
# - Linking common library (ImGui impls, debugdraw)
# - Linking all required libraries (SDL2, bgfx, PE, Boost)
# - Setting up include directories
# - Configuring compiler flags
# - Setting up shaders (copy commands)
# - Configuring debugger working directory
# - Platform-specific setup (Windows DLL copy, Emscripten config)

function(add_bgfx_application)
    # Parse arguments
    cmake_parse_arguments(
        APP                          # Prefix
        ""                           # Options (boolean flags)
        "NAME"                       # One-value keywords
        "SOURCES"                    # Multi-value keywords
        ${ARGN}
    )

    # Validate required arguments
    if(NOT APP_NAME)
        message(FATAL_ERROR "add_bgfx_application: NAME is required")
    endif()

    if(NOT APP_SOURCES)
        message(FATAL_ERROR "add_bgfx_application: SOURCES is required")
    endif()

    # Create executable
    add_executable(${APP_NAME})
    target_sources(${APP_NAME} PRIVATE ${APP_SOURCES})
    target_compile_features(${APP_NAME} PRIVATE cxx_std_11)

    # Link common library (ImGui impls + debugdraw)
    target_link_libraries(${APP_NAME} PRIVATE bgfx-common)

    # Link all required external libraries
    target_link_libraries(
        ${APP_NAME} PRIVATE
        SDL2::SDL2-static
        SDL2::SDL2main
        bgfx::bgfx
        bgfx::bx
        imgui.cmake::imgui.cmake
    )

    # Link PE library if found
    if (PE_LIBRARY)
        target_link_libraries(${APP_NAME} PRIVATE ${PE_LIBRARY})
        # PE also needs Boost
        find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
        target_link_libraries(${APP_NAME} PRIVATE
            Boost::thread
            Boost::system
            Boost::filesystem
            Boost::program_options
        )
    endif()

    # Add PE include directory
    if (PE_INCLUDE_DIR)
        target_include_directories(${APP_NAME} PRIVATE ${PE_INCLUDE_DIR})
    endif()

    # Stop on first error for easier debugging
    target_compile_options(${APP_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wfatal-errors>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )

    # Emscripten-specific link options
    target_link_options(
        ${APP_NAME} PRIVATE
        $<$<BOOL:${EMSCRIPTEN}>:-sMAX_WEBGL_VERSION=2
        -sALLOW_MEMORY_GROWTH=1
        --preload-file=shader/embuild/v_simple.bin
        --preload-file=shader/embuild/f_simple.bin>
    )

    target_compile_definitions(
        ${APP_NAME} PRIVATE
        $<$<BOOL:${EMSCRIPTEN}>:USE_SDL=2>
    )

    # Emscripten-specific setup
    if (EMSCRIPTEN)
        set_target_properties(${APP_NAME} PROPERTIES SUFFIX ".html")
        add_custom_command(
            TARGET ${APP_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    $<TARGET_FILE_DIR:${APP_NAME}>/shader/embuild
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/shader/embuild/v_simple.bin
                $<TARGET_FILE_DIR:${APP_NAME}>/shader/embuild
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/shader/embuild/f_simple.bin
                $<TARGET_FILE_DIR:${APP_NAME}>/shader/embuild
            VERBATIM
        )
    else()
        # Copy shaders for non-Emscripten builds
        add_custom_command(
            TARGET ${APP_NAME}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/shader/build
                $<TARGET_FILE_DIR:${APP_NAME}>/shader/build
            VERBATIM
        )
    endif()

    # Set Visual Studio debugger working directory
    set_target_properties(
        ${APP_NAME}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    )

    # Windows-specific: copy SDL2.dll
    if (WIN32)
        add_custom_command(
            TARGET ${APP_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL2::SDL2>
                    $<TARGET_FILE_DIR:${APP_NAME}>
            VERBATIM
        )
    endif()

endfunction()
