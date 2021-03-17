add_executable(sample_decode
    src/utils.cpp
    src/sample_decode.cpp)

target_link_libraries(sample_decode
    PUBLIC NvCodec)