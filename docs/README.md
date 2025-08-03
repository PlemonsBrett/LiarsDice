# LiarsDice Documentation

This directory contains the documentation for the LiarsDice C++23 implementation.

## 📚 Documentation Structure

```sh
docs/
├── README.md                 # This file
├── api/                      # API documentation
│   ├── ai.md                # AI system documentation
│   ├── configuration.md     # Configuration system documentation
│   ├── core.md              # Core game mechanics documentation
│   └── logging.md           # Logging system documentation
├── architecture/             # System architecture documentation
│   ├── ai-system.md         # AI architecture and design
│   └── logging-and-configuration.md  # Logging and config architecture
├── development/              # Developer guides
│   └── logging-and-configuration-guide.md  # Implementation guide
├── technical/                # Technical decisions and analysis
│   ├── commit-1-di-architecture.md         # Dependency injection design
│   ├── commit-2-testing-framework.md       # Testing framework setup
│   ├── commit-3-logging-system.md          # Logging implementation
│   ├── commit-4-configuration-system.md    # Configuration design
│   ├── commit-5-validation-design.md       # Input validation design
│   └── commit-6-ai-strategy.md            # AI strategy patterns
└── design/                   # Design documents (empty)
```

## 📖 Documentation Overview

This documentation is organized as Markdown files providing comprehensive information about the LiarsDice C++23
implementation. The documentation covers API references, system architecture, development guides, and technical design
decisions.

## 📖 Documentation Content

### API Documentation

- **[Core API](api/core.md)**: Core game mechanics and interfaces
- **[AI System](api/ai.md)**: AI strategy patterns and implementation
- **[Configuration](api/configuration.md)**: Configuration system API
- **[Logging](api/logging.md)**: Logging system API

### Architecture Documentation

- **[AI System Architecture](architecture/ai-system.md)**: AI design patterns and implementation
- **[Logging & Configuration](architecture/logging-and-configuration.md)**: System infrastructure design

### Technical Documentation

- **[DI Architecture](technical/commit-1-di-architecture.md)**: Dependency injection design
- **[Testing Framework](technical/commit-2-testing-framework.md)**: Test infrastructure setup
- **[Logging System](technical/commit-3-logging-system.md)**: Logging implementation details
- **[Configuration System](technical/commit-4-configuration-system.md)**: Configuration design
- **[Validation Design](technical/commit-5-validation-design.md)**: Input validation patterns
- **[AI Strategy](technical/commit-6-ai-strategy.md)**: AI strategy implementation

### Development Guides

- **[Logging & Configuration Guide](development/logging-and-configuration-guide.md)**: Implementation guide for
  developers

## 🛠️ Contributing to Documentation

### Adding New Documentation

1. Create Markdown files in the appropriate subdirectory:
    - `api/` for API documentation
    - `architecture/` for system design documentation
    - `technical/` for technical decisions and analysis
    - `development/` for developer guides

2. Follow the existing structure and formatting patterns

3. Use clear, descriptive filenames that match the content

### Style Guidelines

- Use Markdown format for all documentation
- Include a clear heading structure
- Add code examples with proper syntax highlighting
- Link to related documentation where appropriate
- Keep documentation concise but comprehensive

## 📝 Content Guidelines

### Technical Writing

- **Clear and Concise**: Direct, actionable language
- **Code Examples**: Include working C++20 code snippets
- **Cross-References**: Link to related documentation
- **Consistency**: Follow established patterns and terminology

### Code Documentation

- **API Documentation**: Document public interfaces and their usage
- **Implementation Details**: Explain design decisions and rationale
- **Examples**: Provide practical usage examples
- **Performance**: Note performance characteristics where relevant

## 🔍 Viewing Documentation

The documentation can be viewed directly in GitHub or any Markdown viewer:

1. Navigate to the `docs/` directory in the repository
2. Click on any `.md` file to view it rendered in GitHub
3. Use the directory structure above to find specific topics

### Local Viewing

For local viewing with a Markdown preview:

```bash
# Using VS Code
code docs/

# Using any Markdown viewer
open docs/README.md  # macOS
```
