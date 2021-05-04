#FROM nvcr.io/nvidia/tensorrt:19.10-py3
FROM nvidia/cuda:10.2-cudnn7-devel-ubuntu18.04

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends git curl wget ninja-build build-essential gdb python3-tk python3-pip libqt5x11extras5 python3-setuptools python3-dev

RUN DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends software-properties-common
# RUN add-apt-repository -y ppa:jonathonf/ffmpeg-4
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends ffmpeg libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev

ADD ./Video_Codec_SDK_11.0.10.tar.xz /git/Video_Codec_SDK/
ENV PATH_TO_SDK=/git/Video_Codec_SDK/Video_Codec_SDK_11.0.10

RUN pip3 install -U pip
RUN pip3 install black pylint cmake pytest
RUN pip3 install numpy opencv-python

RUN pip3 install torch torchvision
#RUN pip3 install torch==1.7.1+cu101 torchvision==0.8.2+cu101 torchaudio==0.7.2 -f https://download.pytorch.org/whl/torch_stable.html

RUN pip install ruamel.yaml

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

RUN groupmod --gid $USER_GID $USERNAME \
    && usermod --uid $USER_UID --gid $USER_GID $USERNAME \
    && chown -R $USER_UID:$USER_GID /home/$USERNAME
