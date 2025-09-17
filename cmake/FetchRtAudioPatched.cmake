function(fetch_rtaudio_patched)
  set(RTAUDIO_VERSION "6.0.1")
  set(RTAUDIO_URL "https://github.com/thestk/rtaudio/archive/refs/tags/${RTAUDIO_VERSION}.zip")
  set(RTAUDIO_HASH "MD5=bbc1822ae87755f2337cf1f0385fbd84")

  FetchContent_Populate(
    rtaudio_direct
    QUIET
    URL      "${RTAUDIO_URL}"
    URL_HASH "${RTAUDIO_HASH}"
    DOWNLOAD_EXTRACT_TIMESTAMP YES
  )

  set(cmake_file "${rtaudio_direct_SOURCE_DIR}/CMakeLists.txt")
  file(READ "${cmake_file}" contents)

  string(REGEX REPLACE
    "list\\(APPEND LINKLIBS \\$\\{jack_LIBRARIES\\}\\)"
    "if(jack_LINK_LIBRARIES)\n    list(APPEND LINKLIBS \${jack_LINK_LIBRARIES})\n  else()\n    list(APPEND LINKLIBS \${jack_LIBRARIES})\n  endif()"
    contents "${contents}"
  )

  file(WRITE "${cmake_file}" "${contents}")

  FetchContent_Declare(
    rtaudio
    SOURCE_DIR "${rtaudio_direct_SOURCE_DIR}"
    BINARY_DIR "${rtaudio_direct_BINARY_DIR}"
  )

  set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(rtaudio)
endfunction()
