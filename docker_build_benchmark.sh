docker build . -f benchmark.dockerfile -t vtpl/grabber
docker run -it --rm --gpus all -e "NVIDIA_DRIVER_CAPABILITIES=video,compute,utility" vtpl/grabber 1 -1