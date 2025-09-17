cmake_minimum_required(VERSION 3.15)

project(PianoTiles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type por defecto (si no lo pasas desde la CLI)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)
include(FetchRtAudioPatched)

# RtAudio: elegir API según plataforma
if(WIN32)
  set(RTAUDIO_API_WASAPI ON  CACHE BOOL "" FORCE)
  set(RTAUDIO_API_DS     OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_ASIO   OFF CACHE BOOL "" FORCE) # requiere SDK ASIO
  set(RTAUDIO_API_ALSA   OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_PULSE  OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_JACK   OFF CACHE BOOL "" FORCE)
elseif(APPLE)
  set(RTAUDIO_API_CORE   ON  CACHE BOOL "" FORCE) # CoreAudio
  set(RTAUDIO_API_ALSA   OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_PULSE  OFF CACHE BOOL "" FORCE)
  set(RTAUDIO_API_JACK   OFF CACHE BOOL "" FORCE)
else() # Linux
  set(RTAUDIO_API_ALSA   ON  CACHE BOOL "" FORCE)
  set(RTAUDIO_API_PULSE  ON  CACHE BOOL "" FORCE)
  set(RTAUDIO_API_JACK   OFF CACHE BOOL "" FORCE)
endif()

set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS     OFF CACHE BOOL "" FORCE)

fetch_rtaudio_patched()

# GLFW: preferir sistema; si no está, usar FetchContent
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
  message(STATUS "GLFW no encontrado en el sistema; usando FetchContent")

  # Linux: usa X11 y desactiva Wayland para evitar dependencias extra
  set(GLFW_BUILD_X11      ON  CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
  # macOS/Windows ignoran estas opciones; GLFW detecta plataforma

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
  # El paquete glfw3 suele exportar el target 'glfw'
  set(GLFW_TARGET glfw)
endif()

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/VirtualPiano.cpp
  # (Opcional, para que aparezcan en el IDE)
  # src/VirtualPiano.hpp
  # src/Note.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# Warnings útiles
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wconversion -Wno-sign-conversion)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rtaudio
    ${GLFW_TARGET}
    OpenGL::GL
    Threads::Threads
)

# macOS: frameworks (GLFW suele encargarse, pero por si acaso)
if(APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
