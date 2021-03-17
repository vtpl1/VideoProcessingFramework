#Do version stuff;
set (PYNVCODEC_VERSION_MAJOR 1)
set (PYNVCODEC_VERSION_MINOR 0)

configure_file(
	"inc/Version.hpp.in"
	"inc/Version.hpp"
)

add_subdirectory(pybind11-2.1.1)
add_subdirectory(TC)

pybind11_add_module(PyNvCodec 
    src/PyNvCodec.cpp)

target_include_directories(PyNvCodec 
    PUBLIC inc)

target_link_libraries(PyNvCodec
    PUBLIC TC)

target_compile_definitions(PyNvCodec
    PRIVATE GENERATE_PYTHON_BINDINGS)

add_library(NvCodec SHARED
    src/PyNvCodec.cpp)

target_include_directories(NvCodec 
    PUBLIC inc)

target_link_libraries(NvCodec
    PUBLIC TC)

install(FILES $<TARGET_FILE:PyNvCodec>	                                DESTINATION bin)