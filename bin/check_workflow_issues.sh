#!/bin/bash

# Diagnostic script to identify potential GitHub Actions workflow issues

echo "================================================"
echo "GitHub Actions Workflow Diagnostic Check"
echo "================================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check 1: C++ Standard Library Headers
echo -e "\n${YELLOW}[Check 1]${NC} Checking for standard library includes..."
if grep -r "std::optional" include/ source/ --include="*.hpp" --include="*.cpp" > /dev/null; then
    if grep -r "#include <optional>" include/ --include="*.hpp" > /dev/null; then
        echo -e "${GREEN}✓${NC} std::optional header is included"
    else
        echo -e "${RED}✗${NC} Missing #include <optional> in headers"
    fi
fi

# Check 2: CMake Target Conflicts
echo -e "\n${YELLOW}[Check 2]${NC} Checking for CMake target guards..."
for file in documentation/CMakeLists.txt standalone/CMakeLists.txt standalone/database-demo/CMakeLists.txt; do
    if [ -f "$file" ]; then
        if grep -q "if.*NOT TARGET" "$file"; then
            echo -e "${GREEN}✓${NC} $file has target guards"
        else
            echo -e "${RED}✗${NC} $file missing target guards"
        fi
    fi
done

# Check 3: Python Installation Commands
echo -e "\n${YELLOW}[Check 3]${NC} Checking Python installation commands..."
for workflow in .github/workflows/*.yml .github/workflows/*.yaml; do
    if [ -f "$workflow" ]; then
        name=$(basename "$workflow")
        if grep -q "runs-on:.*macos" "$workflow"; then
            if grep -q "pip.*install" "$workflow"; then
                if grep -q "pip.*install.*--break-system-packages" "$workflow"; then
                    echo -e "${GREEN}✓${NC} $name has --break-system-packages for macOS"
                else
                    echo -e "${RED}✗${NC} $name missing --break-system-packages for macOS pip install"
                fi
            fi
        fi
    fi
done

# Check 4: Build Dependencies
echo -e "\n${YELLOW}[Check 4]${NC} Checking build dependencies..."
echo "Required dependencies:"
echo "  - Boost 1.70+"
echo "  - CMake 3.14+"
echo "  - C++20 compiler"

# Check 5: Test Standalone Build
echo -e "\n${YELLOW}[Check 5]${NC} Testing standalone build configuration..."
if cmake -Sstandalone -Bbuild_test_standalone -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Standalone configuration succeeds"
    rm -rf build_test_standalone
else
    echo -e "${RED}✗${NC} Standalone configuration fails"
fi

# Check 6: Test Documentation Build Configuration
echo -e "\n${YELLOW}[Check 6]${NC} Testing documentation build configuration..."
if cmake -S. -Bbuild_test_docs -DLIARSDICE_BUILD_DOCS=ON -DLIARSDICE_BUILD_TESTS=OFF > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Documentation configuration succeeds"
    rm -rf build_test_docs
else
    echo -e "${RED}✗${NC} Documentation configuration fails"
fi

# Check 7: Installation Test
echo -e "\n${YELLOW}[Check 7]${NC} Testing library installation configuration..."
if cmake -S. -Bbuild_test_install -DCMAKE_BUILD_TYPE=Release -DLIARSDICE_BUILD_TESTS=OFF -DLIARSDICE_BUILD_EXAMPLES=OFF > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Install configuration succeeds"
    rm -rf build_test_install
else
    echo -e "${RED}✗${NC} Install configuration fails"
fi

echo -e "\n================================================"
echo "Diagnostic check complete!"
echo "================================================"
echo ""
echo "Common issues to check:"
echo "1. Windows: Boost package name might be incorrect (boost-msvc-14.3)"
echo "2. macOS: Python packages need --break-system-packages flag"
echo "3. All: Missing #include <optional> for GCC/older compilers"
echo "4. Documentation: Requires jinja2, Pygments Python packages"
echo "5. Standalone: May have CMake target conflicts"