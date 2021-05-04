FROM nvidia/cuda:10.2-cudnn7-devel-ubuntu18.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends ffmpeg
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install git python3-pip
RUN pip3 install -U pip
RUN pip3 install git+https://github.com/vtpl1/check_cuda.git
RUN pip3 install opencv-python
WORKDIR /workfiles
#COPY ./1.AVI ./1.AVI
COPY ./1.AVF ./1.AVF
COPY ./build/sample_decode/benchmark_vpf_avf ./benchmark_vpf_avf
COPY ./build/PyNvCodec/PyNvCodec.cpython-36m-x86_64-linux-gnu.so ./build/PyNvCodec/PyNvCodec.cpython-36m-x86_64-linux-gnu.so
COPY ./benchmark ./benchmark
ENTRYPOINT ["/workfiles/benchmark_vpf_avf"]
#ENTRYPOINT ["/usr/bin/python3", "-m", "benchmark"]