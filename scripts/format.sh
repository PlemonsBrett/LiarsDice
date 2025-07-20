#!/bin/bash

# Script to format C++ code using clang-format
# Usage: ./scripts/format.sh [--check] [--verbose]

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CLANG_FORMAT_PATH="/opt/homebrew/opt/llvm/bin/clang-format"

# Parse arguments
CHECK_ONLY=false
VERBOSE=false
for arg in "$@"; do
    case $arg in
        --check)
            CHECK_ONLY=true
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

# Function to check if clang-format is available
check_clang_format() {
    if ! command -v "$CLANG_FORMAT_PATH" &> /dev/null; then
        echo "❌ Error: clang-format not found at $CLANG_FORMAT_PATH"
        echo "   Please install LLVM or update the path in this script."
        echo "   Try: brew install llvm"
        exit 1
    fi
    
    # Check version
    local version=$("$CLANG_FORMAT_PATH" --version | head -n1)
    log_verbose "Using $version"
}

# Function to check .clang-format configuration
check_config() {
    if [ ! -f "$PROJECT_ROOT/.clang-format" ]; then
        echo "⚠️  Warning: .clang-format configuration file not found"
        echo "   Using default clang-format configuration"
    else
        log_verbose "Using .clang-format configuration from project root"
        if [ "$VERBOSE" = true ]; then
            local standard=$(grep "^Standard:" "$PROJECT_ROOT/.clang-format" | cut -d: -f2 | tr -d ' ')
            log_verbose "C++ Standard: $standard"
        fi
    fi
}

# Function to collect files for formatting
collect_files() {
    local directory="$1"
    local pattern="$2"
    
    if [ -d "$directory" ]; then
        find "$directory" -name "$pattern" -type f 2>/dev/null || true
    fi
}

# Function to format or check a single file
format_file() {
    local file="$1"
    local check_only="$2"
    
    if [ "$check_only" = true ]; then
        # Check if file needs formatting
        if ! "$CLANG_FORMAT_PATH" --dry-run --Werror "$file" &>/dev/null; then
            echo "  ❌ $file (needs formatting)"
            return 1
        else
            if [ "$VERBOSE" = true ]; then
                echo "  ✅ $file (already formatted)"
            fi
            return 0
        fi
    else
        # Format the file in-place
        echo "  📝 Formatting: $file"
        "$CLANG_FORMAT_PATH" -i "$file"
        return 0
    fi
}

# Function to process a collection of files
process_files() {
    local check_only="$1"
    shift
    local files=("$@")
    
    if [ ${#files[@]} -eq 0 ]; then
        echo "   ℹ️  No files to process in this directory."
        return 0
    fi
    
    local action="Formatting"
    if [ "$check_only" = true ]; then
        action="Checking"
    fi
    
    echo "   🔍 $action ${#files[@]} file(s)..."
    
    local error_count=0
    for file in "${files[@]}"; do
        if ! format_file "$file" "$check_only"; then
            ((error_count++))
        fi
    done
    
    if [ "$check_only" = true ] && [ $error_count -gt 0 ]; then
        echo "   ❌ $error_count file(s) need formatting"
        return 1
    elif [ "$check_only" = true ]; then
        echo "   ✅ All files are properly formatted"
    else
        echo "   ✅ All files formatted successfully"
    fi
    
    return 0
}

# Main execution
main() {
    echo "🎨 C++ Code Formatting with clang-format"
    echo "   Project: $(basename "$PROJECT_ROOT")"
    echo "   Standard: C++23"
    echo ""
    
    # Check prerequisites
    check_clang_format
    check_config
    
    # Determine mode
    if [ "$CHECK_ONLY" = true ]; then
        echo "👀 Checking code formatting (no changes will be made)..."
    else
        echo "🔨 Formatting code (files will be modified in-place)..."
    fi
    echo ""
    
    # Change to project root for consistent relative paths
    cd "$PROJECT_ROOT"
    
    local overall_error=0
    
    # Process different file categories
    echo "📂 Processing Header Files..."
    header_files=($(collect_files "include" "*.hpp"))
    if ! process_files "$CHECK_ONLY" "${header_files[@]}"; then
        overall_error=1
    fi
    echo ""
    
    echo "📂 Processing Source Files..."
    src_files=($(collect_files "src" "*.cpp"))
    src_headers=($(collect_files "src" "*.hpp"))
    if ! process_files "$CHECK_ONLY" "${src_files[@]}" "${src_headers[@]}"; then
        overall_error=1
    fi
    echo ""
    
    echo "📂 Processing Application Files..."
    app_files=($(collect_files "apps" "*.cpp"))
    app_headers=($(collect_files "apps" "*.hpp"))
    if ! process_files "$CHECK_ONLY" "${app_files[@]}" "${app_headers[@]}"; then
        overall_error=1
    fi
    echo ""
    
    echo "📂 Processing Example Files..."
    example_files=($(collect_files "examples" "*.cpp"))
    example_headers=($(collect_files "examples" "*.hpp"))
    if ! process_files "$CHECK_ONLY" "${example_files[@]}" "${example_headers[@]}"; then
        overall_error=1
    fi
    echo ""
    
    # Summary
    if [ "$CHECK_ONLY" = true ]; then
        if [ $overall_error -eq 0 ]; then
            echo "✅ All files are properly formatted!"
            echo "💡 Your code follows the project's formatting standards."
        else
            echo "❌ Some files need formatting!"
            echo "💡 Run './scripts/format.sh' to fix formatting issues."
            exit 1
        fi
    else
        echo "✅ Code formatting complete!"
        echo "💡 Next steps:"
        echo "   1. Review changes: git diff"
        echo "   2. Run linting: ./scripts/lint.sh"
        echo "   3. Run tests: ./scripts/test.sh"
    fi
}

# Run main function
main "$@"