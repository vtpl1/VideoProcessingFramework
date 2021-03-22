PATH_TO_SDK=./Video_Codec_SDK_11.0.10
REM FFMPEG_DIR=./ffmpeg-4.3.2-2021-02-20-full_build-shared
FFMPEG_DIR=./ffmpeg
REM del build_release /s /q
mkdir build_release
cd build_release
cmake .. -G"Visual Studio 16 2019" -DVIDEO_CODEC_SDK_DIR="../Video_Codec_SDK_11.0.10" -DFFMPEG_DIR="../ffmpeg" -DCMAKE_INSTALL_PREFIX="../bin"
cmake --build . --config Release
REM cudnn needed, only release build for PytorchNvCodec
cd ..