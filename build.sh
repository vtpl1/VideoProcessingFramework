cmake .. \
  -DVIDEO_CODEC_SDK_DIR:PATH="$PATH_TO_SDK" \
  -DGENERATE_PYTHON_BINDINGS:BOOL="1" \
  -DCMAKE_INSTALL_PREFIX:PATH="$PWD/../install"

cmake .. \
  -DVIDEO_CODEC_SDK_DIR:PATH="$PATH_TO_SDK" \
  -DCMAKE_INSTALL_PREFIX:PATH="$PWD/../install"
make 
make install
python3 SampleDecode.py 0 /workspaces/VideoProcessingFramework/1_2.mp4 1.264