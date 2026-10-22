cmake_minimum_required(VERSION 3.18.1)

project("SE_MP4")

set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/FFMpeg/include")
if (NOT EXISTS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h")
        message(FATAL_ERROR "FFmpeg headers not found. Put them under libmediaprocessing/src/main/cpp/FFMpeg/include.")
endif ()

set(FFMPEG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libffmpeg.so")
if (NOT EXISTS "${FFMPEG_LIBRARY}")
        message(FATAL_ERROR "libffmpeg.so not found for ${ANDROID_ABI}. Put it under libmediaprocessing/src/main/jniLibs/${ANDROID_ABI}/libffmpeg.so.")
endif ()

add_library(ffmpeg SHARED IMPORTED)
set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION "${FFMPEG_LIBRARY}")

add_library(${CMAKE_PROJECT_NAME} SHARED
        mediaprocessing.cpp
        sem4_native.cpp
        mpeg4ip_bitstream.cpp
        mpeg4ip_bitstream_api.cpp
        mp4_manager.cpp
        mp4.cpp
        mp4_read.cpp
        audio_aac.cpp
        Common.cpp
        h265_head.cpp
        libmpeg/src/mpeg-crc32.c
        libmpeg/src/mpeg-element-descriptor.c
        libmpeg/src/mpeg-pack-header.c
        libmpeg/src/mpeg-packet.c
        libmpeg/src/mpeg-pat.c
        libmpeg/src/mpeg-pes.c
        libmpeg/src/mpeg-pmt.c
        libmpeg/src/mpeg-ps-dec.c
        libmpeg/src/mpeg-ps-enc.c
        libmpeg/src/mpeg-psd.c
        libmpeg/src/mpeg-psm.c
        libmpeg/src/mpeg-sdt.c
        libmpeg/src/mpeg-system-header.c
        libmpeg/src/mpeg-ts-dec.c
        libmpeg/src/mpeg-ts-enc.c
        libmpeg/src/mpeg-ts-h264.c
        libmpeg/src/mpeg-ts-h265.c
        libmpeg/src/mpeg-util.c
)

target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/include
        ${FFMPEG_INCLUDE_DIR}
)

target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE
        _GNU_SOURCE
        _FILE_OFFSET_BITS=64
        LINUX
        LINUX_ANDROID
        NDEBUG
        __STDC_CONSTANT_MACROS
)

target_compile_options(${CMAKE_PROJECT_NAME}
        PRIVATE
        -fexceptions
        -Wno-write-strings
        -Wno-deprecated-declarations
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ffmpeg
        android
        log)