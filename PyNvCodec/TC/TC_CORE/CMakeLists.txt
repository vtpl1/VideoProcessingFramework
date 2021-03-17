#Do version stuff;
set (TC_CORE_VERSION_MAJOR 1)
set (TC_CORE_VERSION_MAJOR 0)

configure_file(
	"inc/Version.hpp.in"
	"inc/Version.hpp"
)

add_library(TC_CORE STATIC
	src/Task.cpp
	src/Token.cpp
)
target_include_directories(TC_CORE
	PUBLIC inc
)

set_property(TARGET TC_CORE PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)

install(FILES $<TARGET_FILE:TC_CORE>									DESTINATION bin)
