REM PATH_TO_SDK=./Video_Codec_SDK_11.0.10
REM FFMPEG_DIR=./ffmpeg-4.4-full_build-shared
REM FFMPEG_DIR=./ffmpeg
REM del build_release /s /q
REM nvcc --version
REM python -m venv venv
REM pip install torch==1.7.1+cu110 torchvision==0.8.2+cu110 -f https://download.pytorch.org/whl/torch_stable.html
mkdir build_release
cd build_release
cmake .. -G"Visual Studio 16 2019" -DVIDEO_CODEC_SDK_DIR="../Video_Codec_SDK_11.0.10" -DFFMPEG_DIR="../ffmpeg" -DCMAKE_INSTALL_PREFIX="../bin"
cmake --build . --config Release
REM cudnn needed, only release build for PytorchNvCodec
cd ..