find_package(Threads)
add_executable(benchmark_vpf_cpu_avf
    src/utils.cpp
    src/AVFReader.cpp
    src/benchmark_vpf_cpu_avf.cpp)

target_link_libraries(benchmark_vpf_cpu_avf
    PUBLIC NvCodecCpu
    PRIVATE Threads::Threads)
