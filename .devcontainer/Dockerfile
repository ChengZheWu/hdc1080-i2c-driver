FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Taipei
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LANGUAGE=en_US:en

RUN apt-get update && apt-get install -y \
    locales \
    sudo \
    build-essential \
    git \
    curl \
    wget \
    vim \
    python3 \
    python3-pip \
    cmake \
    ninja-build \
    pkg-config \
    libglib2.0-dev \
    libfdt-dev \
    libpixman-1-dev \
    libslirp-dev \
    zlib1g-dev \
    bc \
    cpio \
    rsync \
    unzip \
    file \
    bison \
    flex \
    libssl-dev \
    libelf-dev \
    libncurses-dev \
    u-boot-tools \
    device-tree-compiler \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    gdb-multiarch \
    gcc-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf \
    qemu-system-arm \
    diffstat \
    chrpath \
    socat \
    xz-utils \
    debianutils \
    iputils-ping \
    python3-pexpect \
    python3-git \
    python3-jinja2 \
    libsdl1.2-dev \
    pylint \
    xterm \
    mesa-common-dev \
    zstd \
    lz4 \
    libegl1 \
    gawk \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8


RUN pip3 install kas

RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER developer
WORKDIR /workspace
