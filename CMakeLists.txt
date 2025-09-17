cmake_minimum_required(VERSION 3.15)

project(PianoTiles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)
include(FetchRtAudioPatched)

set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS     OFF CACHE BOOL "" FORCE)

set(RTAUDIO_API_ALSA  ON  CACHE BOOL "" FORCE)
set(RTAUDIO_API_PULSE ON  CACHE BOOL "" FORCE)
set(RTAUDIO_API_JACK  OFF CACHE BOOL "" FORCE)

fetch_rtaudio_patched()

find_package(glfw3 QUIET)

if (NOT glfw3_FOUND)
  message(STATUS "GLFW no encontrado en el sistema; usando FetchContent")

  set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

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

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/VirtualPiano.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rtaudio
    ${GLFW_TARGET}
    OpenGL::GL
    Threads::Threads
)