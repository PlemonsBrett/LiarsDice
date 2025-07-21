=============
Configuration
=============

.. contents:: Table of Contents
   :local:
   :depth: 2

Game Configuration
==================

The LiarsDice game engine supports various configuration options for customizing gameplay.

Configuration Files
-------------------

Create a ``game_config.json`` file in your working directory:

.. code-block:: json

   {
       "rules": {
           "starting_dice_per_player": 5,
           "minimum_players": 2,
           "maximum_players": 8
       },
       "ai": {
           "decision_time_limit_ms": 2000,
           "learning_enabled": true
       }
   }

.. seealso::
   - :doc:`getting-started` - Basic setup
   - :doc:`running-games` - Game execution