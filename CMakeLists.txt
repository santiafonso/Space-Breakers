cmake_minimum_required(VERSION 3.16)
project(space_breakers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators (Make, Ninja): default to an optimised build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- SFML -------------------------------------------------------------------
# Use the system SFML if it is installed (fast: the usual case on Linux).
# Otherwise fetch and build 2.6.1 from source and link it statically, so a
# fresh Windows machine needs no SFML install and the .exe ships with just
# openal32.dll beside it.
find_package(SFML 2.5 COMPONENTS graphics window audio system QUIET)
if(NOT SFML_FOUND)
    message(STATUS "SFML not found on the system - fetching and building 2.6.1")
    include(FetchContent)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC      OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK  OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.1
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(SFML)
endif()

# --- the game -------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(space_breakers ${SOURCES})
target_include_directories(space_breakers PRIVATE src)
target_link_libraries(space_breakers PRIVATE
    sfml-graphics sfml-window sfml-audio sfml-system)

if(MSVC)
    target_compile_options(space_breakers PRIVATE /W3 /permissive-)
else()
    target_compile_options(space_breakers PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    # No console window on double-click; sfml-main supplies the entry point.
    set_target_properties(space_breakers PROPERTIES WIN32_EXECUTABLE ON)
    target_link_libraries(space_breakers PRIVATE sfml-main)
endif()

# --- bundle runtime files beside the binary -----------------------------
add_custom_command(TARGET space_breakers POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:space_breakers>/assets"
    VERBATIM)

# OpenAL is a runtime DLL on Windows even when SFML itself is static.
if(WIN32 AND TARGET OpenAL)
    add_custom_command(TARGET space_breakers POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:OpenAL>" "$<TARGET_FILE_DIR:space_breakers>"
        VERBATIM)
endif()

# --- install / package layout -----------------------------------------
install(TARGETS space_breakers RUNTIME DESTINATION .)
install(DIRECTORY assets DESTINATION .)
