#Do version stuff;
set (PYNVCODEC_VERSION_MAJOR 1)
set (PYNVCODEC_VERSION_MINOR 0)

configure_file(
	"inc/Version.hpp.in"
	"inc/Version.hpp"
)

add_subdirectory(pybind11-2.1.1)
add_subdirectory(TC)

pybind11_add_module(PyNvCodecCpu 
    src/PyNvCodec.cpp)

target_include_directories(PyNvCodecCpu 
    PUBLIC inc)

target_link_libraries(PyNvCodecCpu
    PUBLIC TC)

target_compile_definitions(PyNvCodecCpu
    PRIVATE GENERATE_PYTHON_BINDINGS)

add_library(NvCodecCpu STATIC
    src/PyNvCodec.cpp)

target_include_directories(NvCodecCpu 
    PUBLIC inc)

target_link_libraries(NvCodecCpu
    PUBLIC TC)


install(FILES $<TARGET_FILE:PyNvCodecCpu>	                                DESTINATION bin)