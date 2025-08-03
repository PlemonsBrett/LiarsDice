# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a modern C++20 implementation of the Liar's Dice game with Boost libraries. The project features:

- Point-based elimination system (5 starting points, differential point loss)
- AI players with configurable strategies
- Optimized game state storage with bit-packing
- Comprehensive testing with Boost.Test and Robot Framework

## Essential Build Commands

```bash
# Quick build (Release by default)
./build.sh
./build.sh Debug    # Debug build

# Run the game
./build/standalone/liarsdice
./build/standalone/liarsdice --seed 12345    # Deterministic mode

# Run all tests
./test.sh

# Run specific unit test
./build/test/test_dice
./build/test/test_player
./build/test/test_game
./build/test/test_ai
./build/test/test_service_container
./build/test/test_game_state_storage
./build/test/test_data_structures
./build/test/test_statistics
./build/test/test_performance    # Performance benchmarks

# Run Robot Framework tests
./test/robot/run_tests.sh
./test/robot/run_tests.sh --suite edge    # Run specific suite
./test/robot/run_tests.sh --tag smoke     # Run by tag

# Clean build
./clean.sh
```

## High-Level Architecture

### Core Game Flow

1. **Game** class orchestrates gameplay, manages players, validates guesses
2. **Player** hierarchy: HumanPlayer and AIPlayer (EasyAI, MediumAI, HardAI)
3. **Point System**: Players start with 5 points, lose 1 for lying, 2 for false accusation
4. **Validation**: Single source in Game::is_valid_guess() - must increase dice count OR keep same count with higher
   face value

### Key Design Patterns

**Event System (Boost.Signals2)**

- Game emits events through GameEvents struct
- Application layer subscribes to render UI updates
- Loose coupling between game logic and presentation

**Dependency Injection**

- Custom lightweight DI container in di/service_container.hpp
- Used primarily for configuration and logging services

**State Storage Optimization**

- GameStateStorage: Cache-efficient with boost::container::flat_map
- CompactGameState: 8-byte bit-packed representation
- GameHistory: Ring buffer with pool allocator for AI analysis

### Testing Architecture

**Unit Tests (Boost.Test)**

- Located in test/source/
- Each component has a dedicated test file
- Run with ./test.sh or individual executables

**Robot Framework Tests**

- Located in test/robot/
- LiarsDiceLibrary.py provides Python interface
- Tests CLI interaction, input validation, game flow
- Some tests skipped due to timeout issues (marked with Skip)

### AI System

**Strategy Pattern**

- IAIStrategy interface for all AI implementations
- AIStrategyFactory with type erasure
- Configurable parameters (risk tolerance, bluff frequency)

**Current Implementations**
- **EasyAIStrategy**: Simple heuristics with configurable parameters
  - Risk tolerance (0.0–1.0)
  - Bluff frequency
  - Call threshold
  - Statistical analysis toggles
  
- **MediumAIStrategy**: Statistical AI with advanced probability calculations
  - Bayesian probability calculations
  - Opponent behavior modeling with pattern tracking
  - Bluff detection using statistical analysis
  - Pattern recognition algorithms
  - Utilizes `GameHistory` for current game context analysis
  - Configurable pattern weight and history size

- **HardAI**: Advanced opponent modeling with pattern tracking

## Optimized Game State Representation

The project includes an optimized game state storage system designed for efficient memory usage and cache performance:

### GameStateStorage

- Uses `boost::container::flat_map` and `flat_set` for cache-efficient data access
- Provides O(log n) lookup with better cache locality than node-based containers
- Tracks active players separately for quick iteration

### CompactGameState

- Bit-packed representation using only 8 bytes per state
- Stores up to 5 dice values in 15 bits (3 bits per die)
- Player state packed into single byte (points, dice count, active flag)
- Last action information compressed into 2 bytes
- Supports serialization/deserialization for network or storage

### GameHistory

- Ring buffer implementation using `boost::circular_buffer`
- Automatically discards old entries when capacity is reached
- Uses pool allocator for efficient memory management
- Provides analysis methods (dice frequency, average dice count)

### Memory Pool Allocator

- Custom `GameStateAllocator` using `boost::pool`
- Efficient allocation/deallocation for frequently created states
- Reduces memory fragmentation and improves performance

## High-Performance Data Structures

The project includes advanced data structures for game analytics and AI optimization:

### TrieMap

- Efficient pattern storage using trie structure with `boost::container::flat_map`
- O(m) operations where m is pattern length
- Used for player behavior pattern tracking (e.g., "GGCL" sequences)
- Specialized `PlayerPatternTrie` for behavior statistics

### CircularBuffer

- Enhanced `boost::circular_buffer` wrapper with perfect forwarding
- Sliding window analysis and statistical calculations
- Pattern detection within bounded history
- Used for recent game state tracking and moving averages

### SparseMatrix

- Built on `boost::numeric::ublas::compressed_matrix`
- Efficient storage for sparse game statistics
- Row/column operations, matrix multiplication, top-N finding
- Specialized types: `PlayerInteractionMatrix`, `ProbabilityMatrix`

### LRUCache

- Multi-index container with O(1) operations
- Automatic eviction of least recently used entries
- Cache statistics tracking (hit rate, misses, evictions)
- Used for AI decision caching and expensive computation memoization

## Statistical Data Containers

The project includes comprehensive statistical analysis tools using Boost libraries:

### StatisticalAccumulator

- Powered by `boost::accumulators` for single-pass statistics
- Calculates mean, variance, min/max, median, skewness, kurtosis
- Rolling window statistics for recent data analysis
- Specialized `DiceRollAccumulator` for game-specific metrics
- Normality testing and coefficient of variation

### Histogram

- Built on `boost::histogram` with automatic axis type selection
- Statistical analysis: mode, percentiles, entropy
- Specialized `DiceHistogram` with fairness testing
- 2D histograms for correlation analysis
- Efficient binning for large datasets

### TimeSeries

- Uses `boost::circular_buffer` for bounded time-ordered data
- Moving averages (simple and exponential)
- Linear trend detection with regression analysis
- Outlier detection using z-score method
- Autocorrelation and seasonal decomposition
- Specialized `GameMetricsTimeSeries` for performance tracking

### ProbabilityDistribution

- Unified interface for `boost::math` distributions
- Implementations: Normal, Binomial, Poisson, Uniform, Exponential, Beta
- Random sampling with `boost::random`
- Hypothesis testing: Kolmogorov-Smirnov and Chi-square tests
- Bayesian inference utilities for adaptive AI

### Performance Optimization

- **SIMD Operations**: boost.simd for vectorized computations
  - 2-4x speedup for mathematical operations
  - Automatic pack size detection
  - Fallback for non-SIMD data sizes

- **Custom Allocators**: boost::pool based memory management
  - FastPoolAllocator for small objects (3-5x faster)
  - SimdAllocator for aligned memory (32-byte alignment)
  - GameObjectPool for object recycling
  - MemoryArena for bulk temporary allocations

- **Performance Testing**: Comprehensive benchmarking suite
  - boost::timer for nanosecond precision
  - Statistical validation of performance gains
  - Memory usage profiling and tracking

## Key Design Patterns

- **Separation of Concerns**: Library vs. application code
- **Modern CMake**: Target-based configuration with proper visibility
- **RAII**: Resource management with smart pointers
- **Exception Safety**: Custom exception hierarchy for error handling
- **Factory Pattern**: AI strategy creation with type erasure
- **Variant Pattern**: Type-safe AI decisions using std::variant

## Critical Implementation Details

### Guess Validation Rules

```cpp
// In Game::is_valid_guess()
1. Cannot guess more dice than exist in game
2. Must either:
   - Increase dice count (any face value), OR
   - Keep same dice count with higher face value
3. Cannot decrease dice count
```

### Memory Layout (CompactGameState)

- Dice values: 15 bits (3 bits × 5 dice)
- Player state: 8 bits (4 points + 3 dice count + 1 active)
- Last action: 16 bits
- Total: 8 bytes per state

### Robot Framework Integration

- CLI path: build/standalone/liarsdice
- Expects specific prompts: "Enter the number of players," "How many AI players"
- Type conversion: Always use str() in send_input to avoid float/string errors

## Dependencies

- **Boost 1.82.0+**: Core dependency for advanced features
  - boost::container (flat_map, flat_set)
  - boost::numeric::ublas (sparse matrices)
  - boost::circular_buffer (ring buffers)
  - boost::multi_index (LRU cache)
  - boost::accumulators (statistics)
  - boost::histogram (binned data)
  - boost::math (probability distributions)
  - boost::random (random sampling)
  - boost::pool (memory allocation)
  - boost::signals2 (event system)
  - boost::test (unit testing)
  - boost::timer (performance measurement)
  - boost::align (aligned allocation)

- **boost.simd**: SIMD vectorization (via CPM)
  - Added as header-only dependency
  - Provides portable SIMD operations

## Known Issues

1. Robot Framework tests with multiple AIs may timeout
2. Logging can impact performance in tight loops
3. Boost libraries required - ensure full Boost installation

## Development Workflow

1. Make changes to source files
2. Run ./build.sh to rebuild
3. Run ./test.sh to verify all tests pass
4. Update CLAUDE.md when adding new build commands or changing architecture
5. Update docs/technical/ when implementing major features