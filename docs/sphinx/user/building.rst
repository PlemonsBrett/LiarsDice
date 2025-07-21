================
Building the Project
================

.. contents:: Table of Contents
   :local:
   :depth: 2

This guide provides detailed instructions for building the LiarsDice project from source, including advanced configuration options and troubleshooting tips.

Build System Overview
=====================

The project uses a modern CMake-based build system with the following components:

- **CMake 3.28+**: Primary build system generator
- **Conan 2.0**: Dependency management and package resolution
- **C++23 Compiler**: GCC 12+, Clang 15+, or MSVC 2022+
- **Catch2**: Testing framework (managed by Conan)

Quick Build Guide
=================

For most users, the provided build script is the easiest way to get started:

.. code-block:: bash

   # Clone and enter project directory
   git clone https://github.com/bplemons/LiarsDice.git
   cd LiarsDice

   # Build everything (Release configuration)
   ./scripts/build.sh

   # Build with specific configuration
   ./scripts/build.sh Debug

The build script handles dependency installation, CMake configuration, and compilation automatically.

Manual Build Process
====================

For advanced users or development environments, you may prefer manual control:

Step 1: Environment Setup
-------------------------

**Install Build Dependencies:**

.. tabs::

   .. tab:: macOS (Homebrew)

      .. code-block:: bash

         # Install LLVM toolchain
         brew install llvm cmake ninja

         # Install Python and Conan
         brew install python
         pip3 install "conan>=2.0"

   .. tab:: Ubuntu/Debian

      .. code-block:: bash

         # Install build tools
         sudo apt update
         sudo apt install build-essential cmake ninja-build python3-pip

         # Install modern Clang
         wget https://apt.llvm.org/llvm.sh
         chmod +x llvm.sh
         sudo ./llvm.sh 15

         # Install Conan
         pip3 install "conan>=2.0"

   .. tab:: CentOS/RHEL

      .. code-block:: bash

         # Enable EPEL and install development tools
         sudo dnf install epel-release
         sudo dnf groupinstall "Development Tools"
         sudo dnf install cmake ninja-build python3-pip

         # Install Conan
         pip3 install "conan>=2.0"

Step 2: Conan Configuration
---------------------------

**Configure Conan Profile:**

.. code-block:: bash

   # Auto-detect system configuration
   conan profile detect --force

   # Verify profile
   conan profile show default

**Customize Conan Profile (Optional):**

.. code-block:: bash

   # Edit the default profile
   conan profile edit default

Example profile for Clang on macOS:

.. code-block:: ini

   [settings]
   arch=armv8
   build_type=Release
   compiler=clang
   compiler.version=15
   compiler.libcxx=libc++
   os=Macos

   [conf]
   tools.system.package_manager:mode=install
   tools.system.package_manager:sudo=True

Step 3: Dependency Installation
-------------------------------

**Install Project Dependencies:**

.. code-block:: bash

   # Create build directory
   mkdir build && cd build

   # Install dependencies for Release build
   conan install .. --output-folder=. --build=missing -s build_type=Release

   # Or for Debug build with sanitizers
   conan install .. --output-folder=. --build=missing -s build_type=Debug

Step 4: CMake Configuration
---------------------------

**Configure the Build:**

.. code-block:: bash

   # Basic configuration
   cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
            -DCMAKE_BUILD_TYPE=Release

   # Advanced configuration with all options
   cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
            -DCMAKE_BUILD_TYPE=Debug \
            -DLIARSDICE_BUILD_TESTS=ON \
            -DLIARSDICE_BUILD_EXAMPLES=ON \
            -DLIARSDICE_BUILD_BENCHMARKS=ON \
            -DLIARSDICE_BUILD_DOCS=ON \
            -DLIARSDICE_USE_SANITIZERS=ON \
            -DCMAKE_VERBOSE_MAKEFILE=ON

Step 5: Compilation
-------------------

**Build the Project:**

.. code-block:: bash

   # Build with all available CPU cores
   cmake --build . --config Release -j$(nproc)

   # Build specific targets
   cmake --build . --target liarsdice-cli
   cmake --build . --target unit_tests
   cmake --build . --target advanced_tests

   # Build documentation (if enabled)
   cmake --build . --target docs

Build Configuration Options
============================

The project supports numerous CMake configuration options:

Core Options
------------

.. list-table:: Core Build Options
   :header-rows: 1
   :widths: 40 20 40

   * - Option
     - Default
     - Description
   * - CMAKE_BUILD_TYPE
     - Release
     - Build configuration (Debug/Release/RelWithDebInfo/MinSizeRel)
   * - LIARSDICE_BUILD_TESTS
     - ON
     - Enable test suite compilation
   * - LIARSDICE_BUILD_EXAMPLES
     - ON
     - Enable example programs
   * - LIARSDICE_BUILD_BENCHMARKS
     - OFF
     - Enable performance benchmarks
   * - LIARSDICE_BUILD_DOCS
     - OFF
     - Enable documentation generation
   * - LIARSDICE_INSTALL
     - ON
     - Generate installation targets
   * - LIARSDICE_USE_SANITIZERS
     - ON (Debug only)
     - Enable AddressSanitizer and UBSan

Advanced Options
----------------

.. list-table:: Advanced Build Options
   :header-rows: 1
   :widths: 40 20 40

   * - Option
     - Default
     - Description
   * - CMAKE_CXX_COMPILER
     - Auto-detected
     - C++ compiler to use
   * - CMAKE_TOOLCHAIN_FILE
     - None
     - CMake toolchain file (set by Conan)
   * - CMAKE_VERBOSE_MAKEFILE
     - OFF
     - Show detailed build commands
   * - CMAKE_EXPORT_COMPILE_COMMANDS
     - ON
     - Generate compile_commands.json for IDEs

Build Types
===========

Debug Build
-----------

**Characteristics:**
- Debug symbols included
- Optimizations disabled (``-O0``)
- Assertions enabled
- Sanitizers enabled (if ``LIARSDICE_USE_SANITIZERS=ON``)

**Usage:**

.. code-block:: bash

   cmake .. -DCMAKE_BUILD_TYPE=Debug \
            -DLIARSDICE_USE_SANITIZERS=ON

**Best for:**
- Development and debugging
- Running tests with maximum error detection
- Memory leak detection

Release Build
-------------

**Characteristics:**
- Optimizations enabled (``-O3``)
- Debug symbols stripped
- Assertions disabled
- Link-time optimization (LTO) enabled

**Usage:**

.. code-block:: bash

   cmake .. -DCMAKE_BUILD_TYPE=Release

**Best for:**
- Production deployments
- Performance testing
- Distribution packages

RelWithDebInfo Build
--------------------

**Characteristics:**
- Optimizations enabled (``-O2``)
- Debug symbols included
- Good balance of performance and debuggability

**Usage:**

.. code-block:: bash

   cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

**Best for:**
- Production debugging
- Profiling optimized code
- Performance analysis

Compiler-Specific Configurations
=================================

Clang Configuration
-------------------

**Recommended Setup:**

.. code-block:: bash

   # Use latest Clang with libc++
   cmake .. -DCMAKE_CXX_COMPILER=clang++ \
            -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
            -DCMAKE_BUILD_TYPE=Release

**Clang-specific Features:**
- Better error messages and diagnostics
- Clang-tidy static analysis integration
- Comprehensive sanitizer support
- Fast compilation times

GCC Configuration
-----------------

**Recommended Setup:**

.. code-block:: bash

   # Use GCC 12+ for C++23 support
   cmake .. -DCMAKE_CXX_COMPILER=g++-12 \
            -DCMAKE_BUILD_TYPE=Release

**GCC-specific Features:**
- Excellent optimization capabilities
- Strong standards compliance
- Good debugging support with GDB

Cross-Platform Considerations
=============================

macOS Specifics
---------------

**Apple Silicon (M1/M2):**

.. code-block:: bash

   # Ensure ARM64 build
   cmake .. -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -DCMAKE_BUILD_TYPE=Release

**Intel Macs:**

.. code-block:: bash

   # Ensure x86_64 build
   cmake .. -DCMAKE_OSX_ARCHITECTURES=x86_64 \
            -DCMAKE_BUILD_TYPE=Release

Linux Specifics
---------------

**Package Manager Integration:**

.. code-block:: bash

   # Enable system package manager for Conan
   conan profile update settings.tools.system.package_manager:mode=install default
   conan profile update settings.tools.system.package_manager:sudo=True default

Windows Specifics
-----------------

**Visual Studio:**

.. code-block:: powershell

   # Configure for Visual Studio 2022
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release

**MinGW:**

.. code-block:: bash

   # Configure for MinGW
   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
   cmake --build . -j8

Performance Optimization
========================

Build Performance
-----------------

**Parallel Compilation:**

.. code-block:: bash

   # Use all CPU cores
   cmake --build . -j$(nproc)

   # Limit to specific number of cores
   cmake --build . -j8

**Ninja Generator:**

.. code-block:: bash

   # Use Ninja for faster builds
   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
   ninja

**Ccache Integration:**

.. code-block:: bash

   # Enable ccache for faster rebuilds
   cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

Runtime Performance
-------------------

**Link-Time Optimization:**

.. code-block:: bash

   # Enable LTO for maximum performance
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

**Profile-Guided Optimization:**

.. code-block:: bash

   # First build with profiling
   cmake .. -DCMAKE_CXX_FLAGS="-fprofile-generate"
   cmake --build .
   
   # Run typical workload to generate profile data
   ./build/bin/liarsdice-cli < typical_game.input
   
   # Rebuild with profile data
   cmake .. -DCMAKE_CXX_FLAGS="-fprofile-use"
   cmake --build .

Troubleshooting
===============

Common Build Issues
-------------------

**Issue: CMake version too old**

.. code-block:: text

   CMake Error: CMake 3.28 or higher is required. You are running version 3.20.

**Solution:**

.. code-block:: bash

   # Install newer CMake
   pip3 install cmake --upgrade
   # or use package manager
   brew install cmake  # macOS
   sudo apt install cmake  # Ubuntu (may need backports)

**Issue: Conan dependencies fail**

.. code-block:: text

   ConanException: Unable to find 'catch2/3.4.0'

**Solution:**

.. code-block:: bash

   # Update Conan and retry
   pip3 install --upgrade conan
   conan search catch2 --remote=all
   conan install .. --build=missing -r=all

**Issue: Compiler not found**

.. code-block:: text

   CMake Error: CMAKE_CXX_COMPILER not set

**Solution:**

.. code-block:: bash

   # Specify compiler explicitly
   cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
            -DCMAKE_C_COMPILER=/usr/bin/clang

**Issue: C++23 features not supported**

.. code-block:: text

   error: 'std::expected' is not a member of 'std'

**Solution:**

.. code-block:: bash

   # Ensure C++23 support
   clang++ --version  # Should be 15+
   g++ --version      # Should be 12+
   
   # Update compiler if needed
   sudo apt install clang-15  # Ubuntu
   brew install llvm          # macOS

Memory and Disk Issues
----------------------

**Issue: Out of memory during compilation**

.. code-block:: bash

   # Reduce parallel jobs
   cmake --build . -j2

   # Or use less memory-intensive build
   cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel

**Issue: Insufficient disk space**

.. code-block:: bash

   # Clean build directory
   rm -rf build/
   
   # Clean Conan cache
   conan remove "*" --confirm

Performance Debugging
----------------------

**Enable build timing:**

.. code-block:: bash

   # Time compilation phases
   cmake .. -DCMAKE_CXX_FLAGS="-ftime-trace"

**Profile build performance:**

.. code-block:: bash

   # Use ninja with timing
   cmake .. -G Ninja
   ninja -j1 -v -d stats

Validation and Testing
======================

Build Verification
------------------

After building, verify the installation:

.. code-block:: bash

   # Check executable exists and runs
   ./build/bin/liarsdice-cli --version

   # Run basic tests
   ./build/bin/unit_tests

   # Run all tests
   ctest --output-on-failure --parallel 4

Installation Testing
--------------------

.. code-block:: bash

   # Install to temporary prefix
   cmake --build . --target install -- DESTDIR=/tmp/test-install

   # Verify installation
   ls /tmp/test-install/usr/local/bin/
   ls /tmp/test-install/usr/local/include/liarsdice/

Documentation Generation
------------------------

.. code-block:: bash

   # Generate all documentation
   cmake --build . --target docs

   # Generate API docs only
   cmake --build . --target doxygen

   # Generate user docs only
   cmake --build . --target sphinx-html

.. seealso::
   - :doc:`getting-started` - Quick start guide
   - :doc:`../development/testing` - Running and writing tests
   - :doc:`../development/contributing` - Development workflow