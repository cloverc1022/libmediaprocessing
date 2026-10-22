cmake_minimum_required(VERSION 3.18)

project(SE_MP4)

get_filename_component(CPP_ROOT ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)

if(NOT DEFINED OHOS_ARCH)
    set(OHOS_ARCH arm64-v8a)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CPP_ROOT}/output/ohos/release/${OHOS_ARCH})

set(FFMPEG_INCLUDE_DIR "${CPP_ROOT}/FFMpeg/include")
set(FFMPEG_LIB_DIR "${CPP_ROOT}/../jniLibs/ohos/${OHOS_ARCH}")

if(DEFINED FFMPEG_LIBRARY)
    set(_ffmpeg_mono "${FFMPEG_LIBRARY}")
else()
    set(_ffmpeg_mono "${FFMPEG_LIB_DIR}/libffmpeg.so")
endif()

if(NOT EXISTS "${FFMPEG_INCLUDE_DIR}/libavcodec/avcodec.h")
    message(FATAL_ERROR "FFmpeg headers missing under ${FFMPEG_INCLUDE_DIR}")
endif()

set(_ffmpeg_link_targets "")

if(EXISTS "${_ffmpeg_mono}")
    add_library(ffmpeg SHARED IMPORTED)
    set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION "${_ffmpeg_mono}")
    list(APPEND _ffmpeg_link_targets ffmpeg)
else()
    foreach(_fflib avcodec avformat avutil swresample)
        set(_path "${FFMPEG_LIB_DIR}/lib${_fflib}.so")
        if(NOT EXISTS "${_path}")
            message(FATAL_ERROR
                    "Missing OHOS FFmpeg: put libffmpeg.so at ${_ffmpeg_mono}\n"
                    "or libavcodec/libavformat/libavutil/libswresample .so under ${FFMPEG_LIB_DIR}")
        endif()
        add_library(${_fflib} SHARED IMPORTED)
        set_target_properties(${_fflib} PROPERTIES IMPORTED_LOCATION "${_path}")
        list(APPEND _ffmpeg_link_targets ${_fflib})
    endforeach()
endif()

add_library(${PROJECT_NAME} SHARED
        ${CPP_ROOT}/mediaprocessing_napi.cpp
        ${CPP_ROOT}/sem4_native.cpp
        ${CPP_ROOT}/mpeg4ip_bitstream.cpp
        ${CPP_ROOT}/mpeg4ip_bitstream_api.cpp
        ${CPP_ROOT}/mp4_manager.cpp
        ${CPP_ROOT}/mp4.cpp
        ${CPP_ROOT}/mp4_read.cpp
        ${CPP_ROOT}/audio_aac.cpp
        ${CPP_ROOT}/Common.cpp
        ${CPP_ROOT}/h265_head.cpp
        ${CPP_ROOT}/libmpeg/src/mpeg-crc32.c
        ${CPP_ROOT}/libmpeg/src/mpeg-element-descriptor.c
        ${CPP_ROOT}/libmpeg/src/mpeg-pack-header.c
        ${CPP_ROOT}/libmpeg/src/mpeg-packet.c
        ${CPP_ROOT}/libmpeg/src/mpeg-pat.c
        ${CPP_ROOT}/libmpeg/src/mpeg-pes.c
        ${CPP_ROOT}/libmpeg/src/mpeg-pmt.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ps-dec.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ps-enc.c
        ${CPP_ROOT}/libmpeg/src/mpeg-psd.c
        ${CPP_ROOT}/libmpeg/src/mpeg-psm.c
        ${CPP_ROOT}/libmpeg/src/mpeg-sdt.c
        ${CPP_ROOT}/libmpeg/src/mpeg-system-header.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ts-dec.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ts-enc.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ts-h264.c
        ${CPP_ROOT}/libmpeg/src/mpeg-ts-h265.c
        ${CPP_ROOT}/libmpeg/src/mpeg-util.c)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CPP_ROOT}
        ${CPP_ROOT}/include
        ${CPP_ROOT}/libmpeg/include
        ${FFMPEG_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        BUILD_FOR_OHOS=1
        LINUX_ANDROID
        _GNU_SOURCE
        _FILE_OFFSET_BITS=64
        LINUX
        NDEBUG
        __STDC_CONSTANT_MACROS)

target_compile_options(${PROJECT_NAME}
        PRIVATE
        -fexceptions
        -Wno-write-strings
        -Wno-deprecated-declarations
        -Wall
        -Os)

target_link_libraries(${PROJECT_NAME}
        ${_ffmpeg_link_targets}
        ace_napi.z
        c++_shared
        -Wl,--no-undefined)
