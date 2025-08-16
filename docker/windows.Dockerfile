# Windows workflow environment - using Wine on Linux to simulate
# Note: Full Windows testing requires Windows containers or VMs
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Wine and Windows build tools
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
    wine64 \
    wine32 \
    build-essential \
    cmake \
    ninja-build \
    mingw-w64 \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Boost for cross-compilation
RUN apt-get update && apt-get install -y libboost-all-dev

WORKDIR /workspace
COPY . .

# Cross-compile for Windows using MinGW
# Note: This is not exactly like MSVC but helps catch cross-platform issues
CMD ["bash", "-c", "\
    cmake -Bbuild -H. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
    -G Ninja && \
    cmake --build build -j4 \
"]