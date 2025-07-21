===============
Getting Started
===============

.. contents:: Table of Contents
   :local:
   :depth: 2

Welcome to the LiarsDice Game Engine! This guide will help you get up and running quickly with building, configuring, and playing the game.

Prerequisites
=============

System Requirements
-------------------

**Operating Systems:**
- macOS 10.15+ (Catalina or later)
- Linux (Ubuntu 20.04+, CentOS 8+, or equivalent)
- Windows 10/11 (with WSL2 recommended)

**Required Software:**

.. list-table:: Build Dependencies
   :header-rows: 1
   :widths: 30 20 50

   * - Software
     - Version
     - Purpose
   * - CMake
     - 3.21+
     - Build system
   * - C++ Compiler
     - C++23 support
     - GCC 12+, Clang 15+, or MSVC 2022+
   * - Conan
     - 2.0+
     - Dependency management
   * - Python
     - 3.8+
     - Build scripts and tools

**Optional Software:**

.. list-table:: Optional Dependencies
   :header-rows: 1
   :widths: 30 20 50

   * - Software
     - Version
     - Purpose
   * - Doxygen
     - 1.9+
     - API documentation generation
   * - Sphinx
     - 4.0+
     - User documentation
   * - Git
     - 2.30+
     - Version control
   * - Ninja
     - 1.10+
     - Fast build system (optional)

Installation
============

Quick Setup (Recommended)
--------------------------

1. **Clone the Repository:**

   .. code-block:: bash

      git clone https://github.com/bplemons/LiarsDice.git
      cd LiarsDice

2. **Install Dependencies:**

   .. code-block:: bash

      # Install Conan if not already installed
      pip install "conan>=2.0"
      
      # Configure Conan profile
      conan profile detect --force

3. **Build the Project:**

   .. code-block:: bash

      # Use the provided build script
      ./scripts/build.sh

4. **Run Tests:**

   .. code-block:: bash

      # Verify everything works
      ./scripts/test.sh

5. **Start Playing:**

   .. code-block:: bash

      # Launch the game
      ./build/bin/liarsdice-cli

Detailed Build Process
----------------------

If you prefer manual control over the build process:

**Step 1: Configure Conan Dependencies**

.. code-block:: bash

   # Create and navigate to build directory
   mkdir build && cd build
   
   # Install dependencies with Conan
   conan install .. --output-folder=. --build=missing -c tools.system.package_manager:mode=install

**Step 2: Configure CMake**

.. code-block:: bash

   # Configure with CMake (from build directory)
   cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DLIARSDICE_BUILD_TESTS=ON \
            -DLIARSDICE_BUILD_EXAMPLES=ON

**Step 3: Build**

.. code-block:: bash

   # Build the project
   cmake --build . --config Release -j$(nproc)

**Step 4: Test**

.. code-block:: bash

   # Run the test suite
   ctest --output-on-failure

Build Options
=============

The project supports several CMake build options:

.. list-table:: CMake Build Options
   :header-rows: 1
   :widths: 40 20 40

   * - Option
     - Default
     - Description
   * - LIARSDICE_BUILD_TESTS
     - ON
     - Build test suite
   * - LIARSDICE_BUILD_EXAMPLES
     - ON
     - Build example programs
   * - LIARSDICE_BUILD_BENCHMARKS
     - OFF
     - Build performance benchmarks
   * - LIARSDICE_BUILD_DOCS
     - OFF
     - Build documentation
   * - LIARSDICE_INSTALL
     - ON
     - Generate install targets
   * - LIARSDICE_USE_SANITIZERS
     - ON (Debug)
     - Enable sanitizers in debug builds

**Example with Custom Options:**

.. code-block:: bash

   cmake .. -DLIARSDICE_BUILD_BENCHMARKS=ON \
            -DLIARSDICE_BUILD_DOCS=ON \
            -DCMAKE_BUILD_TYPE=Debug

First Game
==========

Basic Gameplay
--------------

Once you've built the project, you can start your first game:

.. code-block:: bash

   ./build/bin/liarsdice-cli

**Game Setup:**

1. **Choose Number of Players**: Enter 2-8 players
2. **Select Player Types**: Human or AI (Beginner/Intermediate/Expert)
3. **Configure Rules**: Use defaults or customize

**Sample Game Session:**

.. code-block:: text

   ================================
   Welcome to Liar's Dice!
   ================================
   
   Enter number of players (2-8): 3
   
   Player 1 - Type (h)uman or (a)i: h
   Player 1 - Enter your name: Alice
   
   Player 2 - Type (h)uman or (a)i: a
   Player 2 - AI Level (b)eginner, (i)ntermediate, (e)xpert: i
   
   Player 3 - Type (h)uman or (a)i: a  
   Player 3 - AI Level (b)eginner, (i)ntermediate, (e)xpert: b
   
   ================================
   Game Setup Complete!
   ================================
   Players: Alice (Human), AI-Intermediate, AI-Beginner
   Starting dice per player: 5
   
   Round 1 - Rolling dice...
   
   Alice's turn:
   Your dice: [3] [1] [5] [2] [6]
   Total dice on table: 15
   Last guess: None
   
   Enter your guess (count value) or 'liar': 3 2

**Game Commands:**

.. list-table:: Available Commands
   :header-rows: 1
   :widths: 30 70

   * - Command
     - Description
   * - ``count value``
     - Make a guess (e.g., "4 3" = four dice showing 3)
   * - ``liar``
     - Call the previous player a liar
   * - ``help``
     - Show available commands
   * - ``rules``
     - Display game rules
   * - ``quit``
     - Exit the game

Understanding the Output
------------------------

**Game State Display:**

.. code-block:: text

   Round 3 - Alice's turn:
   Your dice: [2] [2] [4] [1]     # Your remaining dice
   Total dice on table: 8        # All players' dice combined
   Last guess: 3 dice showing 5  # Previous player's guess
   
   Options:
   - Make a higher guess (4+ dice showing 5, or 3+ dice showing 6)
   - Call 'liar' if you think the guess is impossible

**AI Decision Display:**

.. code-block:: text

   AI-Intermediate's turn:
   Analyzing game state... (Bayesian inference)
   Confidence: 0.72
   Decision: 4 dice showing 3
   
   AI-Beginner's turn:
   Thinking... (Simple probability)
   Decision: Calls liar!

Example Programs
================

The project includes several example programs to demonstrate different features:

Basic Game Example
------------------

**File**: ``examples/basic_game.cpp``

.. code-block:: cpp

   #include "liarsdice/core/game.hpp"
   #include "liarsdice/di/service_container.hpp"
   
   int main() {
       using namespace liarsdice;
       
       // Set up dependency injection
       auto container = di::ServiceContainer{};
       container.configure_defaults();
       
       // Create game instance
       auto game = container.resolve<core::IGame>().value();
       
       // Configure for 2 players
       game->initialize();
       game->add_player(1);  // Human player
       game->add_player(2);  // AI player
       
       // Start playing
       game->start_game();
       
       return 0;
   }

Dependency Injection Example
-----------------------------

**File**: ``examples/di_example.cpp``

Demonstrates advanced dependency injection usage:

.. code-block:: cpp

   #include "liarsdice/di/service_container.hpp"
   #include "liarsdice/interfaces/interfaces.hpp"
   
   int main() {
       using namespace liarsdice;
       
       // Create container
       auto container = di::ServiceContainer{};
       
       // Register custom random generator
       container.register_service<interfaces::IRandomGenerator, 
                                 CustomSeededGenerator>(42u);
       
       // Register custom game state
       container.register_service<interfaces::IGameState,
                                 PersistentGameState>("game_data.json");
       
       // Create game with custom dependencies
       auto game = container.resolve<interfaces::IGame>().value();
       
       // Game now uses your custom implementations
       game->initialize();
       
       return 0;
   }

Testing Examples
----------------

**File**: ``examples/testing_example.cpp``

Shows how to use the testing framework:

.. code-block:: cpp

   #include "liarsdice/testing/mock_generator.hpp"
   #include "liarsdice/core/dice_impl.hpp"
   
   int main() {
       using namespace liarsdice;
       
       // Create predictable random generator for testing
       auto mock_rng = std::make_unique<testing::MockRandomGenerator>(
           std::vector<int>{1, 2, 3, 4, 5, 6}  // Predetermined sequence
       );
       
       // Create dice with mock generator
       auto dice = core::DiceImpl{std::move(mock_rng)};
       
       // Dice will roll the predetermined sequence
       for (int i = 0; i < 6; ++i) {
           dice.roll();
           std::cout << "Rolled: " << dice.get_face_value() << "\\n";
       }
       
       return 0;
   }

Configuration
=============

Game Configuration
------------------

Create a ``game_config.json`` file to customize game behavior:

.. code-block:: json

   {
       "rules": {
           "starting_dice_per_player": 5,
           "minimum_players": 2,
           "maximum_players": 8,
           "ones_are_wild": true,
           "exact_call_rule": false
       },
       "ai": {
           "decision_time_limit_ms": 2000,
           "learning_enabled": true,
           "difficulty_adaptation": false
       },
       "display": {
           "show_ai_reasoning": true,
           "animation_speed": "normal",
           "color_scheme": "default"
       }
   }

Build Configuration
-------------------

Create a ``conanfile.txt`` to customize dependencies:

.. code-block:: ini

   [requires]
   catch2/3.4.0
   
   [options]
   catch2:with_main=True
   
   [generators]
   CMakeDeps
   CMakeToolchain

Development Environment
-----------------------

**VS Code Configuration** (```.vscode/settings.json```):

.. code-block:: json

   {
       "cmake.configureArgs": [
           "-DLIARSDICE_BUILD_TESTS=ON",
           "-DLIARSDICE_BUILD_EXAMPLES=ON"
       ],
       "cmake.buildArgs": [
           "-j8"
       ],
       "files.associations": {
           "*.hpp": "cpp"
       }
   }

Next Steps
==========

Now that you have the basics working, you might want to:

1. **Explore the Architecture**: Read :doc:`../architecture/overview`
2. **Learn the API**: Browse :doc:`../api/core`
3. **Contribute**: See :doc:`../development/contributing`
4. **Advanced Features**: Check out :doc:`../data/ai-enhancements`

Troubleshooting
===============

Common Issues
-------------

**Build Fails with "Conan not found":**

.. code-block:: bash

   # Install Conan
   pip install "conan>=2.0"
   
   # Verify installation
   conan --version

**CMake Configuration Fails:**

.. code-block:: bash

   # Clean build directory
   rm -rf build/
   mkdir build && cd build
   
   # Try again with verbose output
   conan install .. --build=missing -v

**Tests Fail to Run:**

.. code-block:: bash

   # Check that tests were built
   ls build/bin/
   
   # Run tests manually
   ./build/bin/unit_tests
   ./build/bin/advanced_tests

**Game Crashes on Startup:**

.. code-block:: bash

   # Check for missing assets
   ls assets/
   
   # Run with debug info
   gdb ./build/bin/liarsdice-cli

Getting Help
------------

- **Documentation**: Browse this documentation for detailed guides
- **Examples**: Check the ``examples/`` directory for working code
- **Issues**: Report bugs on the project repository
- **Community**: Join discussions on the project forums

.. seealso::
   - :doc:`building` - Detailed build instructions
   - :doc:`configuration` - Advanced configuration options
   - :doc:`../development/contributing` - How to contribute to the project