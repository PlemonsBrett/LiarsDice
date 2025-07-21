# LiarsDice Documentation

This directory contains the source files and configuration for the LiarsDice project documentation.

## 📚 Documentation Structure

```
docs/
├── README.md                 # This file
├── requirements.txt          # Python dependencies for building docs
├── sphinx/                   # Sphinx documentation source
│   ├── conf.py              # Sphinx configuration
│   ├── index.rst            # Main documentation index
│   ├── _static/             # Custom CSS and assets
│   ├── user/                # User guides and tutorials
│   ├── architecture/        # System design documentation
│   ├── api/                 # API reference (stubs, actual content from Doxygen)
│   ├── development/         # Developer guides
│   ├── technical/           # Technical decisions and analysis
│   └── data/                # Data models, UML diagrams, schemas
├── api/                     # Generated Doxygen API documentation (build output)
└── html/                    # Generated Sphinx HTML documentation (build output)
```

## 🏗️ Building Documentation

### Prerequisites

```bash
# Install Python dependencies
pip install -r requirements.txt

# Install Doxygen (for API documentation)
brew install doxygen        # macOS
sudo apt install doxygen    # Ubuntu/Debian
```

### Build Commands

```bash
# From project root directory:

# Build all documentation (requires CMake setup)
cmake -B build -S . -DLIARSDICE_BUILD_DOCS=ON
cmake --build build --target docs

# Build only user documentation (Sphinx)
cmake --build build --target sphinx-html

# Build only API documentation (Doxygen)  
cmake --build build --target doxygen

# Alternative: Build Sphinx directly
cd docs/sphinx
sphinx-build -b html . ../../build/docs/html
```

### View Documentation

```bash
# Serve locally
python -m http.server 8080 -d build/docs/html

# Then open http://localhost:8080
```

## 🚀 GitHub Pages Deployment

The documentation is automatically deployed to GitHub Pages via GitHub Actions:

### Automatic Deployment

- **Primary Workflow**: `.github/workflows/docs-simple.yml` (lightweight, reliable)
- **Alternative Workflow**: `.github/workflows/docs.yml` (full CMake build, manual trigger only)
- **Trigger**: Push to `main` or `Enhancements-SoftwareEngineeringAndDesign` branches
- **Output**: Available at `https://[username].github.io/LiarsDice/`

### Manual Deployment

```bash
# Build documentation
cmake --build build --target docs

# Deploy to GitHub Pages (requires gh CLI)
gh workflow run docs.yml
```

## 📖 Documentation Types

### User Documentation (Sphinx)

- **Getting Started**: Setup and basic usage
- **Building**: Detailed build instructions  
- **Configuration**: Game and system configuration
- **Running Games**: Gameplay instructions

### Architecture Documentation

- **Overview**: System architecture and design patterns
- **Dependency Injection**: DI container implementation
- **Interfaces**: Interface design patterns
- **Testing Strategy**: Comprehensive testing approach

### API Reference (Doxygen)

- **Core API**: Game engine interfaces and implementations
- **Interfaces**: Complete interface reference
- **Dependencies**: Dependency injection API
- **Exceptions**: Error handling system

### Technical Documentation

- **C++23 Features**: Modern language features used
- **Performance**: Optimization strategies and considerations
- **Virtual Constexpr**: Technical decision analysis

### Data Models

- **Modern UML**: Current C++23 architecture diagrams
- **Legacy UML**: Historical architecture reference
- **Database Schema**: Data persistence design
- **AI Architecture**: AI enhancement system design

## 🛠️ Contributing to Documentation

### Adding New Pages

1. Create `.rst` files in appropriate `docs/sphinx/` subdirectory
2. Add to relevant `toctree` in parent `index.rst` or section index
3. Follow existing patterns for structure and formatting

### Style Guidelines

- Use reStructuredText (`.rst`) format
- Include table of contents for longer pages
- Use consistent heading hierarchy (`=`, `-`, `~`, `^`)
- Add cross-references between related sections
- Include code examples with proper syntax highlighting

### Testing Documentation

```bash
# Check for syntax errors
sphinx-build -b html -W docs/sphinx build/docs/test

# Check links (requires internet)
sphinx-build -b linkcheck docs/sphinx build/docs/linkcheck

# Validate RST syntax
rstcheck docs/sphinx/**/*.rst
```

## 🎨 Customization

### Themes and Styling

- **Theme**: Sphinx RTD (Read the Docs) theme
- **Custom CSS**: `docs/sphinx/_static/custom.css`
- **Logo**: Place in `docs/sphinx/_static/` and update `conf.py`

### Extensions Used

- **sphinxcontrib.mermaid**: UML and flowchart diagrams
- **sphinx_tabs.tabs**: Tabbed content sections
- **breathe**: Doxygen integration for API docs
- **myst_parser**: Markdown support (if needed)

### Configuration

Main configuration in `docs/sphinx/conf.py`:

- Project metadata
- Extension configuration  
- Theme customization
- Cross-reference settings
- Build options

## 📝 Content Guidelines

### Technical Writing

- **Clear and Concise**: Direct, actionable language
- **Code Examples**: Include working code snippets
- **Cross-References**: Link related concepts
- **Visual Aids**: Use diagrams for complex concepts

### Code Documentation

- **API Docs**: Generated from source code comments
- **Examples**: Include practical usage examples
- **Error Handling**: Document exceptions and error conditions
- **Performance**: Note performance characteristics where relevant

## 🔍 Troubleshooting

### Common Issues

**Sphinx build fails**:
```bash
# Check dependencies
pip install -r docs/requirements.txt

# Clean build
rm -rf build/docs/
cmake --build build --target sphinx-html
```

**Mermaid diagrams not rendering**:
```bash
# Ensure extension is installed
pip install sphinxcontrib-mermaid
```

**Missing cross-references**:
- Check file paths in `:doc:` directives
- Ensure target files exist and are in toctree

**GitHub Pages deployment fails**:
- Check workflow file syntax
- Verify repository has Pages enabled
- Check workflow permissions in repository settings

### Getting Help

- Check [Sphinx documentation](https://www.sphinx-doc.org/)
- Review [reStructuredText primer](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html)
- See existing documentation files for examples
- Check GitHub Actions logs for deployment issues