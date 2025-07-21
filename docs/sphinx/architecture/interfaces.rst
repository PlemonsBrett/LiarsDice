=================
Interface Design
=================

.. contents:: Table of Contents
   :local:
   :depth: 2

Interface-Based Architecture
============================

The modern LiarsDice implementation uses interface-based design for maximum testability and flexibility.

Core Principles
---------------

- Abstract interfaces define contracts
- Implementations are injected via DI
- Mock implementations for testing
- Multiple implementations possible

Interface Hierarchy
====================

The system defines several key interfaces:

Game Interfaces
---------------

- ``IGame`` - Main game controller
- ``IGameState`` - Game state management
- ``IPlayer`` - Player behavior
- ``IDice`` - Individual die behavior

Utility Interfaces
------------------

- ``IRandomGenerator`` - Random number generation
- ``IServiceFactory<T>`` - Object creation

Benefits
========

Interface-based design provides:

- **Testability**: Easy mocking and testing
- **Flexibility**: Multiple implementations
- **Maintainability**: Clear contracts
- **Extensibility**: Add new features easily

.. seealso::
   - :doc:`dependency-injection` - DI implementation details
   - :doc:`../data/modern-uml-diagrams` - Interface diagrams