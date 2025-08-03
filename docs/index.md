---
layout: default
title: Home
---

# LiarsDice Documentation

Welcome to the LiarsDice documentation! This is a modern C++ implementation of the classic Liar's Dice game using Boost
libraries.

## Quick Links

<div class="alert alert-info">
  <strong>Getting Started?</strong> Check out the <a href="{{ site.baseurl }}/README">README</a> for build instructions and basic usage.
</div>

### API Reference

- [Core Game API]({{ site.baseurl }}/api/core) - Core game mechanics and classes
- [AI System API]({{ site.baseurl }}/api/ai) - AI player implementations and strategies
- [Configuration API]({{ site.baseurl }}/api/configuration) - Configuration management system
- [Logging API]({{ site.baseurl }}/api/logging) - Logging infrastructure

### Architecture Documentation

- [AI System Architecture]({{ site.baseurl }}/architecture/ai-system) - Design and implementation of the AI system
- [Logging & Configuration]({{ site.baseurl }}/architecture/logging-and-configuration) - Infrastructure design

### Development Guides

- [Logging & Configuration Guide]({{ site.baseurl }}/development/logging-and-configuration-guide) - Implementation guide
  for developers

### Technical Design Documents

- [DI Architecture]({{ site.baseurl }}/technical/commit-1-di-architecture) - Dependency injection design
- [Testing Framework]({{ site.baseurl }}/technical/commit-2-testing-framework) - Test infrastructure
- [Logging System]({{ site.baseurl }}/technical/commit-3-logging-system) - Logging implementation
- [Configuration System]({{ site.baseurl }}/technical/commit-4-configuration-system) - Configuration design
- [Validation Design]({{ site.baseurl }}/technical/commit-5-validation-design) - Input validation
- [AI Strategy]({{ site.baseurl }}/technical/commit-6-ai-strategy) - AI implementation details

## Project Overview

LiarsDice is a comprehensive implementation featuring:

- 🎲 **Core Game Engine** - Complete game logic with Boost.Signals2 event system
- 🤖 **AI Players** - Multiple difficulty levels with configurable strategies
- 🎮 **Interactive CLI** - Menu-driven interface with input validation
- 🔧 **Dependency Injection** - Modern DI container for flexible architecture
- 📊 **Logging System** - Boost.Log integration for debugging and monitoring
- ⚙️ **Configuration** - Boost.PropertyTree-based configuration system
- 🧪 **Comprehensive Testing** - Unit tests, integration tests, and benchmarks

## Technology Stack

<div class="badge badge-primary">C++20</div>
<div class="badge badge-secondary">Boost</div>
<div class="badge badge-success">CMake</div>
<div class="badge badge-warning">Catch2</div>

### Key Libraries

- **Boost.Signals2** - Event-driven architecture
- **Boost.Random** - Random number generation
- **Boost.Log** - Logging infrastructure
- **Boost.PropertyTree** - Configuration management
- **Catch2** - Testing framework

## Contributing

See the [Development Guide]({{ site.baseurl }}/development/logging-and-configuration-guide) for information on:

- Code style and conventions
- Build system details
- Testing requirements
- Contribution guidelines

## License

This project is part of an academic portfolio. See the repository for license details.