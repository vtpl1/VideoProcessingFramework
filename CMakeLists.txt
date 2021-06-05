cmake_minimum_required(VERSION 3.17)

project(vpf LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (WIN32)
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/ffmpeg CACHE PATH "Path to FFMpeg")
endif()
#VIDEO_CODEC_SDK_DIR="../Video_Codec_SDK_11.0.10" -DFFMPEG_DIR="../ffmpeg
add_subdirectory(PyNvCodec)
add_subdirectory(sample_decode)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecode.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleEncode.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecodeSw.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecodeMultiThread.py	DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleEncodeMultiThread.py	DESTINATION bin)
