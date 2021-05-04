add_executable(sample_decode
    src/utils.cpp
    src/sample_decode.cpp)

target_link_libraries(sample_decode
    PUBLIC NvCodec)

add_executable(benchmark_vpf
    src/utils.cpp
    src/benchmark_vpf.cpp)

target_link_libraries(benchmark_vpf
    PUBLIC NvCodec)

add_executable(benchmark_vpf_avf
    src/utils.cpp
    src/AVFReader.cpp
    src/benchmark_vpf_avf.cpp)

target_link_libraries(benchmark_vpf_avf
    PUBLIC NvCodec)

set_property(TARGET sample_decode PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)

set_property(TARGET benchmark_vpf PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)

set_property(TARGET benchmark_vpf_avf PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)