cmake_minimum_required(VERSION 3.17)

project(vpf LANGUAGES CUDA CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(PyNvCodec)
add_subdirectory(PytorchNvCodec)
add_subdirectory(sample_decode)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecode.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleEncode.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecodeSw.py				DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleDecodeMultiThread.py	DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SampleEncodeMultiThread.py	DESTINATION bin)
