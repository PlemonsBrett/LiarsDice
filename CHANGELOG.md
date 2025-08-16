## [Unreleased]


## [0.1.0] - 2025-08-16

### Added
- GitHub Actions release workflow with automatic changelog generation
- Automatic artifact building for Linux and macOS platforms
- Release asset management with downloadable game executables
- Changelog generation from conventional commits
- Cleanup of old development releases

### Fixed
- Standalone executable exit codes for CI compatibility
- Help command now correctly returns exit code 0
- CI pipeline formatting requirements

### Changed
- Temporarily disabled Windows CI builds due to MinGW/MSVC incompatibility

## [1.0.0] - 2025-08-16

### Added
- Complete Liar's Dice game implementation with Boost libraries
- Point-based elimination system (5 starting points)
- AI players with configurable strategies (Easy, Medium, Hard)
- Optimized game state storage with bit-packing
- Comprehensive testing with Boost.Test and Robot Framework
- Event system using Boost.Signals2
- Dependency injection container
- Advanced data structures for game analytics
- Statistical analysis tools using Boost libraries
- Performance optimizations with SIMD operations
- Database support with SQLite3
- Bayesian analysis for AI decision making
- Memory pool allocators for efficient resource management
- Cross-platform support (Linux, macOS)
- Deterministic mode with seed support for testing
- Extensive documentation with Doxygen

### Game Rules
- Players start with 5 dice and 5 points
- Lose 1 point for lying, 2 points for false accusation
- Must increase dice count OR keep same count with higher face value
- Last player with points remaining wins

### Technical Features
- Modern C++20 implementation
- Cache-efficient data structures with boost::container
- Sparse matrices for game statistics
- LRU cache for AI decision caching
- Circular buffers for game history
- Trie-based pattern matching for behavior tracking
- Comprehensive performance benchmarking suite
- GitHub Actions CI/CD pipeline
- Platform-specific workflows for Ubuntu and macOS
- Code coverage reporting with Codecov
- Automatic documentation deployment

---

For older changes, see the [GitHub releases page](https://github.com/PlemonsBrett/LiarsDice/releases).
