============================
LiarsDice Game Engine
============================

Welcome to the documentation for the **LiarsDice Game Engine**, a modern C++23 implementation of the classic Liar's Dice game featuring dependency injection, comprehensive testing, and advanced software engineering practices.

.. toctree::
   :maxdepth: 2
   :caption: User Guide
   :name: user-guide

   user/getting-started
   user/building
   user/running-games
   user/configuration

.. toctree::
   :maxdepth: 2
   :caption: Architecture & Design
   :name: architecture

   architecture/overview
   architecture/dependency-injection
   architecture/interfaces
   architecture/testing-strategy
   architecture/design-patterns

.. toctree::
   :maxdepth: 2
   :caption: API Reference
   :name: api-reference

   api/core
   api/interfaces
   api/dependency-injection
   api/exceptions

.. toctree::
   :maxdepth: 2
   :caption: Development
   :name: development

   development/contributing
   development/coding-standards
   development/build-system
   development/testing
   development/documentation

.. toctree::
   :maxdepth: 1
   :caption: Technical Decisions
   :name: technical-decisions

   technical/constexpr-virtual-limitations
   technical/cpp23-features
   technical/performance-considerations

.. toctree::
   :maxdepth: 1
   :caption: Data Models & Architecture
   :name: data-models

   data/modern-uml-diagrams
   data/uml-diagrams
   data/database-schema
   data/ai-enhancements

Project Overview
================

The LiarsDice Game Engine is a comprehensive implementation of the classic dice game, built with modern C++23 features and software engineering best practices. The project emphasizes:

* **Modern C++23**: Leveraging the latest language features including concepts, ranges, and modules
* **Dependency Injection**: Clean, testable architecture with IoC container
* **Comprehensive Testing**: Unit tests, integration tests, and property-based testing with Catch2
* **API Documentation**: Generated with Doxygen and integrated with Sphinx
* **Build System**: Modern CMake with Conan 2.0 for dependency management

Key Features
============

Game Engine
-----------

* **Classic Liar's Dice Rules**: Full implementation of traditional gameplay
* **Multiplayer Support**: 2-8 players with both human and AI participants
* **Configurable Rules**: Customizable game parameters and variations
* **Real-time Statistics**: Game analytics and performance metrics

Technical Features
------------------

* **Dependency Injection Container**: Type-safe IoC with C++23 concepts
* **Interface-Based Design**: Clean separation of concerns with abstract interfaces
* **Exception Safety**: Comprehensive error handling with custom exception hierarchy
* **Memory Management**: RAII principles with smart pointers throughout
* **Thread Safety**: Concurrent-safe components where applicable

Development Tools
-----------------

* **Modern CMake**: Target-based configuration with proper visibility
* **Conan 2.0**: Dependency management with reproducible builds
* **Catch2 Testing**: BDD-style tests with property-based testing
* **Clang Toolchain**: Latest compiler with comprehensive warnings
* **Documentation**: Integrated API docs and user guides

Quick Start
===========

Building the Project
---------------------

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/bplemons/LiarsDice.git
   cd LiarsDice

   # Build with the provided script
   ./scripts/build.sh

   # Run tests
   ./scripts/test.sh

   # Run the game
   ./build/bin/liarsdice-cli

For detailed build instructions, see :doc:`user/building`.

Basic Usage
-----------

.. code-block:: cpp

   #include "liarsdice/core/game.hpp"
   #include "liarsdice/di/service_container.hpp"

   using namespace liarsdice;

   int main() {
       // Set up dependency injection
       auto container = di::ServiceContainer{};
       
       // Create and configure game
       auto game = container.resolve<core::IGame>().value();
       game->initialize();
       game->add_player(1);
       game->add_player(2);
       
       // Start playing
       game->start_game();
       
       return 0;
   }

For complete examples, see :doc:`user/getting-started`.

License and Contributing
========================

This project is developed as part of academic coursework and follows industry best practices for modern C++ development. For contribution guidelines, see :doc:`development/contributing`.

Indices and Tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`