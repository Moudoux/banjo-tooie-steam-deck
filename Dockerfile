FROM ubuntu:24.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies including clang, llvm, and lld
RUN apt-get update && apt-get install -y \
    build-essential \
    binutils-mips-linux-gnu \
    gcc-mips-linux-gnu \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-venv \
    python3-pip \
    libfmt-dev \
    libtoml11-dev \
    zlib1g-dev \
    wget \
    tar \
    clang \
    llvm \
    lld \
    pkg-config \
    xz-utils \
    libsdl2-dev \
    libx11-dev \
    libxrandr-dev \
    libvulkan-dev \
    libgtk-3-dev \
    libssl-dev \
    libc++-dev \
    libc++abi-dev \
    && rm -rf /var/lib/apt/lists/*

# Wine runs the hash-pinned Windows LLVM tools that guest preparation expects
# (the Windows build reaches them through WSL interop instead).
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
    wine64 \
    wine \
    && rm -rf /var/lib/apt/lists/*
ENV WINEDEBUG=-all \
    WINEPREFIX=/opt/wineprefix
RUN wineboot --init >/dev/null 2>&1 || true

# Fix Git ownership permissions inside the container
RUN git config --global --add safe.directory '*'

WORKDIR /workspace
