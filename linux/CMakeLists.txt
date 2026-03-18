cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "flutter_soloud")
project(${PROJECT_NAME} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_NAME "${PROJECT_NAME}_plugin")

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")

# Include common plugin sources and SoLoud configuration
include(${SRC_DIR}/CMakeLists.txt)

add_library(${PLUGIN_NAME} SHARED
  ${PLUGIN_SOURCES}
)
apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)
target_compile_definitions(${PLUGIN_NAME} PRIVATE SIGNALSMITH_USE_PFFFT)
target_include_directories(${PLUGIN_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE PkgConfig::GTK)


# Library linking behavior:
# 
# - NO_OPUS_OGG_LIBS: If set, skips linking Opus/Ogg/Vorbis/FLAC libraries entirely.
#
# - FORCE_PRECOMPILED_LIBS: If set, forces the use of bundled precompiled libraries
#                           (libFLAC.so, libopus.so, libogg.so, libvorbis.so, libvorbisfile.so)
#                           from the libs/ directory, ignoring system-installed versions.
#
# - Default behavior (neither env var set): Prefer system libraries if available,
#                                           fall back to bundled precompiled libraries
#                                           if not found on the system.
#
if(NOT DEFINED ENV{NO_OPUS_OGG_LIBS})
    message( "NO_OPUS_OGG_LIBS has not been set. Linking Opus and Ogg libraries!")

    # Check if we should force use of precompiled libraries
    if(DEFINED ENV{FORCE_PRECOMPILED_LIBS})
        message( "FORCE_PRECOMPILED_LIBS is set. Using bundled precompiled libraries.")
        set(USE_PRECOMPILED_LIBS TRUE)
    else()
        message( "FORCE_PRECOMPILED_LIBS is not set. Will prefer system libraries if available.")
        set(USE_PRECOMPILED_LIBS FALSE)
    endif()

    # FLAC
    if(USE_PRECOMPILED_LIBS)
        set(FLAC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libFLAC.so)
        message( "FLAC: using bundled library (forced): ${FLAC_LIBRARY}")
    else()
        find_library(FLAC_LIBRARY NAMES FLAC)
        if(NOT FLAC_LIBRARY)
            set(FLAC_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libFLAC.so)
            message( "FLAC: system library not found, using bundled: ${FLAC_LIBRARY}")
        else()
            message( "FLAC: using system library: ${FLAC_LIBRARY}")
        endif()
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${FLAC_LIBRARY})

    # Opus
    if(USE_PRECOMPILED_LIBS)
        set(OPUS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libopus.so)
        message( "Opus: using bundled library (forced): ${OPUS_LIBRARY}")
    else()
        find_library(OPUS_LIBRARY NAMES opus)
        if(NOT OPUS_LIBRARY)
            set(OPUS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libopus.so)
            message( "Opus: system library not found, using bundled: ${OPUS_LIBRARY}")
        else()
            message( "Opus: using system library: ${OPUS_LIBRARY}")
        endif()
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${OPUS_LIBRARY})

    # Ogg
    if(USE_PRECOMPILED_LIBS)
        set(OGG_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libogg.so)
        message( "Ogg: using bundled library (forced): ${OGG_LIBRARY}")
    else()
        find_library(OGG_LIBRARY NAMES ogg)
        if(NOT OGG_LIBRARY)
            set(OGG_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libogg.so)
            message( "Ogg: system library not found, using bundled: ${OGG_LIBRARY}")
        else()
            message( "Ogg: using system library: ${OGG_LIBRARY}")
        endif()
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${OGG_LIBRARY})

    # Vorbis
    if(USE_PRECOMPILED_LIBS)
        set(VORBIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libvorbis.so)
        message( "Vorbis: using bundled library (forced): ${VORBIS_LIBRARY}")
    else()
        find_library(VORBIS_LIBRARY NAMES vorbis)
        if(NOT VORBIS_LIBRARY)
            set(VORBIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libvorbis.so)
            message( "Vorbis: system library not found, using bundled: ${VORBIS_LIBRARY}")
        else()
            message( "Vorbis: using system library: ${VORBIS_LIBRARY}")
        endif()
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${VORBIS_LIBRARY})

    # Vorbisfile
    if(USE_PRECOMPILED_LIBS)
        set(VORBISFILE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libvorbisfile.so)
        message( "Vorbisfile: using bundled library (forced): ${VORBISFILE_LIBRARY}")
    else()
        find_library(VORBISFILE_LIBRARY NAMES vorbisfile)
        if(NOT VORBISFILE_LIBRARY)
            set(VORBISFILE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libvorbisfile.so)
            message( "Vorbisfile: system library not found, using bundled: ${VORBISFILE_LIBRARY}")
        else()
            message( "Vorbisfile: using system library: ${VORBISFILE_LIBRARY}")
        endif()
    endif()
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${VORBISFILE_LIBRARY})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    message( "NO_OPUS_OGG_LIBS has been set. Not linking Opus and Ogg libraries!")
    add_definitions(-DNO_OPUS_OGG_LIBS)
endif()

target_compile_options("${PLUGIN_NAME}" PRIVATE -Wall -Wno-error -Wno-vla -O2) #  -ldl -lpthread -lm
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    # This only matches x86 and AMD64 architectures.
    # On a Raspberry Pi, will be something like "armv6l", "armv7l" or "aarch64".
    target_compile_options("${PLUGIN_NAME}" PRIVATE -msse3 -msse2)
endif()

# List of absolute paths to libraries that should be bundled with the plugin.
set(flutter_soloud_bundled_libraries
  $<TARGET_FILE:${PLUGIN_NAME}>
  PARENT_SCOPE
)
