# Documentation workflow environment
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Install Boost
RUN apt-get update && apt-get install -y libboost-all-dev

# Install Python packages for documentation
RUN pip3 install jinja2 Pygments sphinx sphinx-rtd-theme breathe

WORKDIR /workspace
COPY . .

# Build documentation
CMD ["bash", "-c", "\
    cmake -S. -Bbuild -DLIARSDICE_BUILD_DOCS=ON -DLIARSDICE_BUILD_TESTS=OFF && \
    cmake --build build --target GenerateDocs \
"]