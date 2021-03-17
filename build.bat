PATH_TO_SDK=./Video_Codec_SDK_11.0.10
FFMPEG_DIR=./ffmpeg-4.3.2-2021-02-20-full_build-shared
mkdir build
cd build
cmake .. -G"Visual Studio 16 2019"
REM cudnn needed, only release build for PytorchNvCodec