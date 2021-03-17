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

install(FILES $<TARGET_FILE:TC_CORE>									DESTINATION bin)
