#Do version stuff;
set (TC_VERSION_MAJOR 1)
set (TC_VERSION_MAJOR 0)

configure_file(
	"inc/Version.hpp.in"
	"inc/Version.hpp"
)


add_subdirectory(TC_CORE)
find_package(CUDAToolkit)

set(VIDEO_CODEC_SDK_DIR "" CACHE PATH "Path to Nvidia Video Codec SDK")
set(FFMPEG_DIR "" CACHE PATH "Path to FFMpeg")

set(FFMPEG_DIR $ENV{FFMPEG_DIR})

if (DEFINED FFMPEG_INCLUDE_DIR)
	set(FFMPEG_INCLUDE_DIR "${FFMPEG_INCLUDE_DIR}" CACHE PATH "Path to FFmpeg includes")
else()
	set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/include")
endif()

if (DEFINED FFMPEG_LIB_DIR)
	set(FFMPEG_LIB_DIR "${FFMPEG_LIB_DIR}" CACHE PATH "Path to FFmpeg libraries")
else()
	set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/lib")
endif()

message(STATUS "Searching for FFmpeg libs in ${FFMPEG_LIB_DIR}")
message(STATUS "Searching for FFmpeg headers in ${FFMPEG_INCLUDE_DIR}")

find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h 
    PATHS ${FFMPEG_INCLUDE_DIR})
find_path( AVFORMAT_INCLUDE_DIR libavformat/avformat.h 
    PATHS ${FFMPEG_INCLUDE_DIR})
find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h 
    PATHS ${FFMPEG_INCLUDE_DIR})

find_library( SWRESAMPLE_LIBRARY swresample 
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVFORMAT_LIBRARY avformat 
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVCODEC_LIBRARY avcodec
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVUTIL_LIBRARY avutil
    PATHS ${FFMPEG_LIB_DIR})

message(STATUS "AVCODEC_INCLUDE_DIR ${AVCODEC_INCLUDE_DIR}")
message(STATUS "SWRESAMPLE_LIBRARY ${SWRESAMPLE_LIBRARY}")

set(VIDEO_CODEC_SDK_DIR $ENV{PATH_TO_SDK})

message(STATUS "Searching for Video Codec SDK headers in ${VIDEO_CODEC_SDK_DIR}/include folder")
find_path( VIDEO_CODEC_SDK_INCLUDE_DIR nvEncodeAPI.h ${VIDEO_CODEC_SDK_DIR}/include)
# Video Codec SDK 10 has headers in folder with different name
if (VIDEO_CODEC_SDK_INCLUDE_DIR-NOTFOUND STREQUAL ${VIDEO_CODEC_SDK_INCLUDE_DIR})
	message(STATUS "Searching for Video Codec SDK headers in ${VIDEO_CODEC_SDK_DIR}/Interface folder")
	find_path( VIDEO_CODEC_SDK_INCLUDE_DIR nvEncodeAPI.h ${VIDEO_CODEC_SDK_DIR}/Interface)
endif()

if(UNIX)
	find_library( NVCUVID_LIBRARY nvcuvid ${VIDEO_CODEC_SDK_DIR}/Lib/linux/stubs/x86_64)
	find_library( NVENCODE_LIBRARY nvidia-encode ${VIDEO_CODEC_SDK_DIR}/Lib/linux/stubs/x86_64)
elseif(WIN32)
	find_library( NVCUVID_LIBRARY nvcuvid ${VIDEO_CODEC_SDK_DIR}/Lib/x64)
	find_library( NVENCODE_LIBRARY nvencodeapi ${VIDEO_CODEC_SDK_DIR}/Lib/x64)
endif(UNIX)
message(STATUS "VIDEO_CODEC_SDK_INCLUDE_DIR ${VIDEO_CODEC_SDK_INCLUDE_DIR}")

add_library(TC STATIC
    src/Resize.cu
    src/FFmpegDemuxer.cpp
    src/FfmpegSwDecoder.cpp
    src/MemoryInterfaces.cpp
    src/NppCommon.cpp
    src/NvCodecCliOptions.cpp
    src/NvDecoder.cpp
    src/NvEncoder.cpp
    src/NvEncoderCuda.cpp
    src/Tasks.cpp
    src/TasksColorCvt.cpp
    src/TasksPreprocess.cpp
)

set_property(TARGET TC PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)

target_include_directories(TC
    PUBLIC inc
    PUBLIC ${AVCODEC_INCLUDE_DIR}
    PUBLIC ${AVFORMAT_INCLUDE_DIR}
    PUBLIC ${AVUTIL_INCLUDE_DIR}
    PUBLIC ${VIDEO_CODEC_SDK_INCLUDE_DIR}
)
# target_compile_definitions(TC
#     PUBLIC CUDA_API_PER_THREAD_DEFAULT_STREAM)

target_compile_definitions(TC
    PUBLIC NOMINMAX)


target_link_libraries(TC
    PUBLIC TC_CORE
    PUBLIC ${SWRESAMPLE_LIBRARY}
    PUBLIC ${AVFORMAT_LIBRARY}
    PUBLIC ${AVCODEC_LIBRARY}
    PUBLIC ${AVUTIL_LIBRARY}
    PUBLIC ${NVCUVID_LIBRARY}
    PUBLIC ${NVENCODE_LIBRARY}
    PUBLIC CUDA::cudart
    PUBLIC CUDA::nppicc
    PUBLIC CUDA::nppig
    PUBLIC CUDA::nppidei
    PUBLIC CUDA::cuda_driver
)

install(FILES $<TARGET_FILE:TC>                                         DESTINATION bin)