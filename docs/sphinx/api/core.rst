========
Core API
========

.. contents:: Table of Contents
   :local:
   :depth: 2

Core Components
===============

The core API provides the fundamental game logic and components.

Game Interface
--------------

.. cpp:class:: liarsdice::interfaces::IGame

   Main game controller interface.

Player Interface
----------------

.. cpp:class:: liarsdice::interfaces::IPlayer

   Player management interface.

Dice Interface
--------------

.. cpp:class:: liarsdice::interfaces::IDice

   Individual die behavior interface.

.. seealso::
   - :doc:`interfaces` - Complete interface reference
   - :doc:`dependency-injection` - DI container API