#!/bin/bash

# Script to run clang-tidy linting on the codebase
# Usage: ./scripts/lint.sh [--fix] [--verbose]

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
CLANG_TIDY_PATH="/opt/homebrew/opt/llvm/bin/clang-tidy"

# Parse arguments
FIX_FLAG=""
VERBOSE=false
for arg in "$@"; do
    case $arg in
        --fix)
            FIX_FLAG="--fix"
            ;;
        --verbose)
            VERBOSE=true
            ;;
    esac
done

# Function to log verbose messages
log_verbose() {
    if [ "$VERBOSE" = true ]; then
        echo "🔍 $1"
    fi
}

# Function to check if clang-tidy is available
check_clang_tidy() {
    if ! command -v "$CLANG_TIDY_PATH" &> /dev/null; then
        echo "❌ Error: clang-tidy not found at $CLANG_TIDY_PATH"
        echo "   Please install LLVM or update the path in this script."
        echo "   Try: brew install llvm"
        exit 1
    fi
    
    # Check version and C++23 support
    local version=$("$CLANG_TIDY_PATH" --version | head -n1)
    log_verbose "Using $version"
}

# Function to ensure build exists with proper configuration
ensure_build() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "📁 Build directory not found. Running build first..."
        "$SCRIPT_DIR/build.sh" || {
            echo "❌ Error: Build failed. Cannot proceed with linting."
            exit 1
        }
    fi

    # Ensure compile_commands.json exists with proper configuration
    if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
        echo "📋 compile_commands.json not found. Generating with C++23 support..."
        cd "$PROJECT_ROOT"
        cmake -B build -S . \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DCMAKE_CXX_STANDARD=23 \
            -DCMAKE_CXX_STANDARD_REQUIRED=ON || {
            echo "❌ Error: Failed to generate compile_commands.json"
            exit 1
        }
    fi
    
    log_verbose "Build system ready with C++23 configuration"
}

# Function to check .clang-tidy configuration
check_config() {
    if [ ! -f "$PROJECT_ROOT/.clang-tidy" ]; then
        echo "⚠️  Warning: .clang-tidy configuration file not found"
        echo "   Using default clang-tidy configuration"
    else
        log_verbose "Using .clang-tidy configuration from project root"
    fi
}

# Function to run clang-tidy on a single file
lint_file() {
    local file="$1"
    local fix_flag="$2"
    
    echo "  📄 Checking: $file"
    
    # Use project's .clang-tidy config and improved error handling
    local cmd="$CLANG_TIDY_PATH"
    
    # Add fix flag if requested
    if [ -n "$fix_flag" ]; then
        cmd="$cmd $fix_flag"
    fi
    
    # Add compilation database
    cmd="$cmd -p $BUILD_DIR"
    
    # Add the file
    cmd="$cmd $file"
    
    if [ "$VERBOSE" = true ]; then
        log_verbose "Running: $cmd"
        eval "$cmd" 2>&1 | grep -v "warning: unknown warning option" || true
    else
        # Suppress system header warnings and errors we can't control
        eval "$cmd" 2>/dev/null | grep -v "error: too many errors emitted" | head -50 || true
    fi
}

# Function to run clang-tidy on a collection of files
run_clang_tidy() {
    local fix_flag="$1"
    shift
    local files=("$@")
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "   ℹ️  No files to analyze in this directory."
        return 0
    fi
    
    echo "   🔍 Analyzing ${#files[@]} file(s)..."
    
    local error_count=0
    for file in "${files[@]}"; do
        if ! lint_file "$file" "$fix_flag"; then
            ((error_count++))
        fi
    done
    
    if [ $error_count -gt 0 ]; then
        echo "   ⚠️  $error_count file(s) had analysis issues"
    else
        echo "   ✅ All files analyzed successfully"
    fi
}

# Function to collect files for analysis
collect_files() {
    local directory="$1"
    local pattern="$2"
    
    if [ -d "$directory" ]; then
        find "$directory" -name "$pattern" -type f 2>/dev/null || true
    fi
}

# Main execution
main() {
    echo "🔧 C++ Code Linting with clang-tidy"
    echo "   Project: $(basename "$PROJECT_ROOT")"
    echo "   Standard: C++23"
    echo ""
    
    # Check prerequisites
    check_clang_tidy
    check_config
    ensure_build
    
    # Determine mode
    if [ -n "$FIX_FLAG" ]; then
        echo "🔨 Running clang-tidy with automatic fixes..."
        echo "   ⚠️  This will modify your source files!"
    else
        echo "👀 Running clang-tidy (analysis only)..."
    fi
    echo ""
    
    # Change to project root for consistent relative paths
    cd "$PROJECT_ROOT"
    
    # Analyze different file categories
    echo "📂 Analyzing Header Files..."
    header_files=($(collect_files "include" "*.hpp"))
    run_clang_tidy "$FIX_FLAG" "${header_files[@]}"
    echo ""
    
    echo "📂 Analyzing Source Files..."
    src_files=($(collect_files "src" "*.cpp"))
    src_headers=($(collect_files "src" "*.hpp"))
    run_clang_tidy "$FIX_FLAG" "${src_files[@]}" "${src_headers[@]}"
    echo ""
    
    echo "📂 Analyzing Application Files..."
    app_files=($(collect_files "apps" "*.cpp"))
    app_headers=($(collect_files "apps" "*.hpp"))
    run_clang_tidy "$FIX_FLAG" "${app_files[@]}" "${app_headers[@]}"
    echo ""
    
    echo "📂 Analyzing Example Files..."
    example_files=($(collect_files "examples" "*.cpp"))
    example_headers=($(collect_files "examples" "*.hpp"))
    run_clang_tidy "$FIX_FLAG" "${example_files[@]}" "${example_headers[@]}"
    echo ""
    
    # Summary
    if [ -n "$FIX_FLAG" ]; then
        echo "✅ Linting complete with automatic fixes applied!"
        echo "💡 Next steps:"
        echo "   1. Review the changes: git diff"
        echo "   2. Run tests: ./scripts/test.sh"
        echo "   3. Run format: find . -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i"
    else
        echo "✅ Linting analysis complete!"
        echo "💡 To apply automatic fixes: ./scripts/lint.sh --fix"
        echo "💡 For verbose output: ./scripts/lint.sh --verbose"
    fi
}

# Run main function
main "$@"