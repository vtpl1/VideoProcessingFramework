#Do version stuff;
set (PYTORCH_NVCODEC_VERSION_MAJOR 1)
set (PYTORCH_NVCODEC_VERSION_MINOR 0)
#execute_process ( COMMAND python3 -c "from distutils.sysconfig import get_python_lib; print(f\"{get_python_lib()}\")" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process ( COMMAND python3 -c "import torch; print(f\"{torch.__path__[0]}\")" OUTPUT_VARIABLE TORCH_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "TORCH_PATH: ${TORCH_PATH}")
list(APPEND CMAKE_PREFIX_PATH ${TORCH_PATH})
find_package(Torch REQUIRED)
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS "${TORCH_INSTALL_PREFIX}/lib")
message(STATUS "TORCH_PYTHON_LIBRARY: ${TORCH_PYTHON_LIBRARY}")
message(STATUS "TORCH_LIBRARIES: ${TORCH_LIBRARIES}")

find_package(CUDAToolkit)

pybind11_add_module(PytorchNvCodec 
	src/PytorchNvCodec.cpp)

target_link_libraries(PytorchNvCodec
	PUBLIC ${TORCH_LIBRARIES}
	PUBLIC ${TORCH_PYTHON_LIBRARY}
	PUBLIC CUDA::cudart)

set_property(TARGET PytorchNvCodec PROPERTY CUDA_ARCHITECTURES 52 53 60 61 62 70 72 75)