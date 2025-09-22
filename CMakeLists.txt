cmake_minimum_required(VERSION 3.15)
project(PianoTiles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Include FetchContent for downloading dependencies
include(FetchContent)

# Configure RtAudio
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Platform-specific RtAudio APIs
if(WIN32)
    set(RTAUDIO_API_WASAPI ON CACHE BOOL "" FORCE)
    set(RTAUDIO_API_DS OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_ASIO OFF CACHE BOOL "" FORCE)
elseif(APPLE)
    set(RTAUDIO_API_CORE ON CACHE BOOL "" FORCE)
else() # Linux
    set(RTAUDIO_API_ALSA OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_PULSE ON CACHE BOOL "" FORCE)
endif()

# Disable unnecessary APIs
set(RTAUDIO_API_JACK OFF CACHE BOOL "" FORCE)

# Fetch RtAudio
FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG 6.0.1
)
FetchContent_MakeAvailable(rtaudio)

# Configure GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Linux-specific GLFW settings
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

# Try to find GLFW in system, otherwise fetch it
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found in system, fetching from source")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
    set(GLFW_TARGET glfw)
else()
    set(GLFW_TARGET glfw)
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/VirtualPiano.cpp
    # Note.hpp is header-only
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall -Wextra -Wno-unused-parameter)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    rtaudio
    ${GLFW_TARGET}
    OpenGL::GL
    Threads::Threads
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
endif()

# Print configuration summary
message(STATUS "=== Virtual Piano Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
if(WIN32)
    message(STATUS "Audio API: WASAPI")
elseif(APPLE)
    message(STATUS "Audio API: CoreAudio")
else()
    message(STATUS "Audio APIs: ALSA, PulseAudio")
endif()