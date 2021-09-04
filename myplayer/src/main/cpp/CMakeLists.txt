
cmake_minimum_required(VERSION 3.10.2)

include_directories(include)
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)

add_library( # Sets the name of the library.
        native-lib2

        SHARED

        native-lib.cpp

        FFmpeg.cpp
        MyAudio.cpp
        SafeQueue.cpp
        PlayStatus.cpp
        CallJava.cpp

        soundtouch/SoundTouch/AAFilter.cpp
        soundtouch/SoundTouch/FIFOSampleBuffer.cpp
        soundtouch/SoundTouch/FIRFilter.cpp
        soundtouch/SoundTouch/cpu_detect_x86.cpp
        soundtouch/SoundTouch/sse_optimized.cpp
        soundtouch/SoundTouch/RateTransposer.cpp
        soundtouch/SoundTouch/SoundTouch.cpp
        soundtouch/SoundTouch/InterpolateCubic.cpp
        soundtouch/SoundTouch/InterpolateLinear.cpp
        soundtouch/SoundTouch/InterpolateShannon.cpp
        soundtouch/SoundTouch/TDStretch.cpp
        soundtouch/SoundTouch/PeakFinder.cpp

        )

add_library(avcodec-57 SHARED IMPORTED)
set_target_properties(avcodec-57
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libavcodec-57.so)


#add_library( avdevice-57 SHARED IMPORTED)
#set_target_properties( avdevice-57
#                       PROPERTIES IMPORTED_LOCATION
#                       ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libavdevice-57.so)
#
#
#add_library( avfilter-6 SHARED IMPORTED)
#set_target_properties( avfilter-6
#                       PROPERTIES IMPORTED_LOCATION
#                       ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libavfilter-6.so)


add_library(avformat-57 SHARED IMPORTED)
set_target_properties(avformat-57
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libavformat-57.so)

add_library(avutil-55 SHARED IMPORTED)
set_target_properties(avutil-55
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libavutil-55.so)

#add_library( postproc-54 SHARED IMPORTED)
#set_target_properties( postproc-54
#                       PROPERTIES IMPORTED_LOCATION
#                       ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libpostproc-54.so)

add_library(swresample-2 SHARED IMPORTED)
set_target_properties(swresample-2
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libswresample-2.so)


add_library(swscale-4 SHARED IMPORTED)
set_target_properties(swscale-4
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/prebuilts/${ANDROID_ABI}/libswscale-4.so)


target_link_libraries( # Specifies the target library.
        native-lib2

        avcodec-57
        avformat-57
        avutil-55
        swresample-2
        swscale-4

        OpenSLES

        # Links the target library to the log library
        # included in the NDK.
        log)