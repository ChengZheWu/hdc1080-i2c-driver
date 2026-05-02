FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Taipei

# 基本工具
RUN apt-get update && apt-get install -y \
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
    && rm -rf /var/lib/apt/lists/*

# ARM bare-metal toolchain (arm-none-eabi)
RUN apt-get update && apt-get install -y \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*

# ARM Linux cross compiler (for kernel)
RUN apt-get update && apt-get install -y \
    gcc-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf \
    && rm -rf /var/lib/apt/lists/*

# QEMU - 從源碼編譯以支援 AST2600
# 先裝 apt 版的 QEMU 支援 mps2-an385（bare-metal/rtos 用）
RUN apt-get update && apt-get install -y \
    qemu-system-arm \
    && rm -rf /var/lib/apt/lists/*

# 建立 developer 使用者
RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER developer
WORKDIR /workspace
