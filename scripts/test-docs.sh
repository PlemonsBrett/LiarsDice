#!/bin/bash
# Test script for documentation build (similar to GitHub Actions)

set -e  # Exit on any error

echo "🔧 Testing Documentation Build Process"
echo "======================================"

# Check if we're in the right directory
if [ ! -f "CLAUDE.md" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    exit 1
fi

echo "📦 Installing Python dependencies..."
pip install -r docs/requirements.txt

echo "🏗️ Creating build directory..."
mkdir -p build/docs

echo "📚 Building Sphinx documentation..."
cd docs/sphinx
sphinx-build -b html . ../../build/docs/html
cd ../..

echo "📖 Building Doxygen documentation..."
if command -v doxygen &> /dev/null; then
    # Create minimal Doxyfile for testing
    cat > Doxyfile.test << 'EOF'
PROJECT_NAME = "LiarsDice Game Engine"
PROJECT_NUMBER = "1.0.0"
OUTPUT_DIRECTORY = build/docs/api
INPUT = include/ src/ apps/ examples/
RECURSIVE = YES
GENERATE_HTML = YES
GENERATE_LATEX = NO
EXTRACT_ALL = YES
QUIET = YES
WARN_IF_UNDOCUMENTED = NO
EOF
    
    doxygen Doxyfile.test
    
    # Integrate API docs
    if [ -d "build/docs/api/html" ]; then
        mkdir -p build/docs/html/api
        cp -r build/docs/api/html/* build/docs/html/api/
        echo "✅ API documentation integrated"
    fi
    
    # Cleanup
    rm -f Doxyfile.test
else
    echo "⚠️  Doxygen not found - skipping API documentation"
fi

echo "🔍 Verifying build output..."
if [ -f "build/docs/html/index.html" ]; then
    echo "✅ Main documentation page exists"
else
    echo "❌ Error: Main documentation page not found"
    exit 1
fi

# Check for key sections
SECTIONS=("user" "architecture" "data" "development" "technical")
for section in "${SECTIONS[@]}"; do
    if [ -d "build/docs/html/$section" ]; then
        echo "✅ $section/ directory exists"
    else
        echo "⚠️  $section/ directory missing"
    fi
done

echo ""
echo "🎉 Documentation build test completed successfully!"
echo ""
echo "📱 To view the documentation locally:"
echo "   python -m http.server 8080 -d build/docs/html"
echo "   Then open: http://localhost:8080"
echo ""
echo "📤 To deploy to GitHub Pages:"
echo "   1. Commit and push the .github/workflows/ files"
echo "   2. Enable Pages in repository settings"
echo "   3. Push to main branch to trigger deployment"