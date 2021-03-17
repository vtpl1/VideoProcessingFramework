add_executable(sample_decode
    src/utils.cpp
    src/sample_decode.cpp)

target_link_libraries(sample_decode
    PUBLIC NvCodec)

set_property(TARGET sample_decode PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)