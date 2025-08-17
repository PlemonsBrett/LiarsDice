@mainpage Liar's Dice Documentation

@tableofcontents

A modern C++20 implementation of the classic dice game with AI players and advanced data structures.

# Overview

Liar's Dice is a dice game where players make increasingly bold claims about the dice hidden under cups, trying to
outbluff their opponents. This implementation features sophisticated AI players, optimized data structures, and a
comprehensive testing framework.

# Key Features

- **Multiple AI Difficulty Levels**: Easy and Medium AI players with different strategies
- **Point-based Elimination System**: Players start with 5 points and lose them based on game outcomes
- **Optimized Game State Storage**: Bit-packed representations and cache-efficient data structures
- **Database Integration**: SQLite-based persistent storage for game history and statistics
- **Comprehensive Testing**: Unit tests with Boost.Test and integration tests with Robot Framework
- **Performance Optimizations**: SIMD operations, custom allocators, and efficient algorithms

# Documentation Sections

## Core Components

- @subpage game_rules "Game Rules" - How to play Liar's Dice
- @subpage architecture "Architecture Overview" - System design and patterns
- @ref liarsdice::core "Core Module" - Core game components

## Game System

- @ref liarsdice::core::Game "Game Class" - Main game orchestration and logic
- @ref liarsdice::core::Player "Player System" - Base player interface and implementations
- @ref liarsdice::core::Dice "Dice Management" - Dice rolling and validation

## AI System

- @subpage ai_system "AI System Overview" - AI architecture and strategies
- @ref liarsdice::ai::AIPlayer "AI Player" - AI player implementation
- @ref liarsdice::ai::EasyAIStrategy "Easy AI" - Simple heuristics-based AI
- @ref liarsdice::ai::MediumAIStrategy "Medium AI" - Statistical analysis AI with pattern recognition

## Data Structures & Optimization

- \ref liarsdice::data_structures "Data Structures" - Advanced containers and algorithms
- \ref liarsdice::data_structures::TrieMap "Trie Map" - Pattern storage and retrieval
- \ref liarsdice::data_structures::LRUCache "LRU Cache" - Efficient caching system
- \ref liarsdice::data_structures::SparseMatrix "Sparse Matrix" - Game statistics storage
- \ref liarsdice::storage::CompactGameState "Compact State" - Bit-packed game representation

## Database System

- @subpage database "Database System" - Complete database documentation
- @ref liarsdice::database::DatabaseManager "Database Manager" - SQLite integration
- @ref liarsdice::database::SchemaManager "Schema Manager" - Database schema and migrations
- @ref liarsdice::database::ConnectionPool "Connection Pool" - Efficient connection management
- Database schema includes tables for:
  - Game sessions and history
  - Player statistics and rankings
  - AI behavior patterns
  - Performance metrics

## Statistical Analysis

- \ref liarsdice::statistics "Statistics Module" - Game analytics
- \ref liarsdice::statistics::StatisticalAccumulator "Accumulator" - Real-time statistics
- \ref liarsdice::statistics::ProbabilityDistribution "Distributions" - Probability calculations
- \ref liarsdice::statistics::TimeSeries "Time Series" - Temporal analysis

## Performance

- \ref liarsdice::performance "Performance Module" - Optimization utilities
- SIMD operations for mathematical computations
- Custom memory allocators for reduced fragmentation
- Object pools for frequently allocated objects

# Getting Started

## Building the Project

```bash
# Quick build (Release mode by default)
./build.sh

# Debug build
./build.sh Debug

# Run the game
./build/standalone/liarsdice
```

## Running Tests

```bash
# All tests
./test.sh

# Specific unit tests
./build/test/test_game
./build/test/test_ai
./build/test/test_database
./build/test/test_database_manager

# Robot Framework tests
./test/robot/run_tests.sh
```

# Architecture & Design

- @subpage architecture_diagrams "Architecture Diagrams" - Visual system design documentation
  - System architecture overview
  - Game flow diagrams
  - AI decision flow
  - Data flow diagrams
  - Class hierarchy
  - Database ERD
  - Memory layout optimization
  - Deployment architecture

# Navigation

Use the navigation menu to explore:

- **Modules** - Organized by functionality
- **Namespaces** - Code organization structure
- **Classes** - All classes with inheritance hierarchy
- **Files** - Source code browser

# Technical Notes

For implementation details and design decisions, see the @subpage technical_notes "Technical Notes" section.