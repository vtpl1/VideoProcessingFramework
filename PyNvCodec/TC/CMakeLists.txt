#Do version stuff;
set (TC_VERSION_MAJOR 1)
set (TC_VERSION_MAJOR 0)

configure_file(
	"inc/Version.hpp.in"
	"inc/Version.hpp"
)


add_subdirectory(TC_CORE)


if (defined $ENV{FFMPEG_DIR})
set(FFMPEG_DIR $ENV{FFMPEG_DIR})
endif()

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

add_library(TC STATIC
    src/FFmpegDemuxer.cpp
    src/FfmpegSwDecoder.cpp
    src/MemoryInterfaces.cpp
    src/Tasks.cpp
    src/TasksParser.cpp
    src/H264BitStreamParser.cpp
)

target_include_directories(TC
    PUBLIC inc
    PUBLIC ${AVCODEC_INCLUDE_DIR}
    PUBLIC ${AVFORMAT_INCLUDE_DIR}
    PUBLIC ${AVUTIL_INCLUDE_DIR}
)

target_compile_definitions(TC
    PUBLIC NOMINMAX
    PUBLIC TRACK_TOKEN_ALLOCATIONS
)


target_link_libraries(TC
    PUBLIC TC_CORE
    PUBLIC ${SWRESAMPLE_LIBRARY}
    PUBLIC ${AVFORMAT_LIBRARY}
    PUBLIC ${AVCODEC_LIBRARY}
    PUBLIC ${AVUTIL_LIBRARY}
)

install(FILES $<TARGET_FILE:TC>                                         DESTINATION bin)