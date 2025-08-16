# Standalone workflow environment
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

# Build standalone executable
CMD ["bash", "-c", "\
    cmake -Sstandalone -Bbuild -DCMAKE_BUILD_TYPE=Release -G Ninja && \
    cmake --build build -j4 && \
    ./build/liarsdice --help && \
    echo '2' | ./build/liarsdice --seed 12345 || true \
"]