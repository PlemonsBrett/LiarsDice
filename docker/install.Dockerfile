# Install workflow environment - tests library installation
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Install Boost
RUN apt-get update && apt-get install -y libboost-all-dev

WORKDIR /workspace
COPY . .

# Test the install workflow commands
CMD ["bash", "-c", "\
    cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DLIARSDICE_BUILD_TESTS=OFF -DLIARSDICE_BUILD_EXAMPLES=OFF && \
    cmake --build build --target install && \
    rm -rf build && \
    cmake -Stest -Bbuild -DTEST_INSTALLED_VERSION=1 && \
    cmake --build build --config Debug -j4 && \
    cd build && ctest --build-config Debug --output-on-failure \
"]