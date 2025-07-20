# Conan 2.0 Setup Guide

This guide helps you set up Conan 2.0 for the LiarsDice project.

## Installation

### Method 1: pip (Recommended)

```bash
# Install Conan
pip install conan

# Verify installation
conan --version
```

### Method 2: Homebrew (macOS)

```bash
# Install Conan
brew install conan

# Verify installation
conan --version
```

### Method 3: Package Managers

```bash
# Ubuntu/Debian
sudo apt-get install python3-pip
pip install conan

# CentOS/RHEL/Fedora
sudo yum install python3-pip
pip install conan

# Windows (with Chocolatey)
choco install conan  # Note: choco is the Chocolatey package manager
```

## Initial Setup

### 1. Detect Profile

```bash
# Auto-detect your system settings
conan profile detect --force
```

### 2. Create Custom Profile (Optional)

```bash
# Copy the project profile
cp profiles/default ~/.conan2/profiles/liarsdice

# Edit the profile to match your system
conan profile show liarsdice
```

### 3. Test Installation

```bash
# Install dependencies for the project
conan install . --build=missing

# Build the project with Conan
./scripts/build-conan.sh
```

## Common Conan Commands

### Project Dependencies

```bash
# Install dependencies
conan install . --build=missing

# Install with specific build type
conan install . --build=missing -s build_type=Debug

# Install with specific profile
conan install . --build=missing --profile=liarsdice
```

### Package Management

```bash
# Search for packages
conan search catch2

# Show package information
conan inspect catch2/3.4.0

# List installed packages
conan list
```

### Building and Creating

```bash
# Build and create package
conan create . --build=missing

# Build with options
conan create . --build=missing -o build_tests=True -o build_examples=False
```

## Project-Specific Options

The LiarsDice project supports these Conan options:

```bash
# Available options
-o shared=True/False          # Build shared libraries
-o fPIC=True/False           # Position independent code
-o build_tests=True/False    # Build test suite
-o build_examples=True/False # Build example programs
-o build_benchmarks=True/False # Build benchmarks
```

### Examples

```bash
# Build without tests
conan install . -o build_tests=False

# Build shared libraries
conan install . -o shared=True

# Build for debugging
conan install . -s build_type=Debug -o build_tests=True
```

## Troubleshooting

### Missing Profiles

```bash
# If you get profile errors
conan profile detect --force
conan profile show default
```

### Build Errors

```bash
# Clean Conan cache
conan remove "*" --force

# Rebuild all dependencies
conan install . --build=missing --build=*
```

### Compiler Issues

```bash
# Check detected settings
conan profile show default

# Manually set compiler  
# Note: libcxx and libstdc are C++ standard library settings
conan profile update settings.compiler=clang default
conan profile update settings.compiler.version=15 default
conan profile update settings.compiler.libcxx=libstdc++11 default
```

### Network Issues

```bash
# If GitHub access fails, try with retries
conan config set general.retry=2
conan config set general.retry_wait=1
```

## Integration with IDEs

### Visual Studio Code

The project automatically works with VS Code when using the CMake extension:

1. Install dependencies: `conan install . --build=missing`
2. Open VS Code: `code .`
3. Select CMake kit when prompted
4. Build with `Ctrl+Shift+P` → "CMake: Build"

### CLion

1. Install dependencies: `conan install . --build=missing`
2. Open project in CLion
3. CLion will automatically detect the CMake configuration

## Advanced Usage

### Multiple Profiles

```bash
# Create profiles for different environments
conan profile detect --force --name=debug
conan profile detect --force --name=release

# Use specific profile
conan install . --profile=debug
```

### Custom Settings

```bash
# Override settings temporarily (cppstd = C++ standard)
conan install . -s compiler.cppstd=23 -s build_type=RelWithDebInfo
```

### Lock Files

```bash
# Generate lock file for reproducible builds
conan lock create . --lockfile-out=conan.lock

# Use lock file
conan install . --lockfile=conan.lock
```

## References

- [Conan Documentation](https://docs.conan.io/2.0/)
- [Conan Getting Started](https://docs.conan.io/2.0/tutorial.html)
- [CMake Integration](https://docs.conan.io/2.0/examples/tools/cmake.html)
