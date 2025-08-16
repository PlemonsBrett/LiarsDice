# Ubuntu workflow environment
FROM ubuntu:22.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install Boost (same as GitHub Actions)
RUN apt-get update && apt-get install -y libboost-all-dev

# Install Python packages (Robot Framework for testing)
RUN pip install robotframework==7.0 pexpect==4.9.0 psutil==5.9.8

# Set working directory
WORKDIR /workspace

# Copy the source code
COPY . .

# Build commands from ubuntu.yml
CMD ["bash", "-c", "cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TEST_COVERAGE=ON -G Ninja && cmake --build build -j4 && cd build && ctest -C Debug --output-on-failure"]