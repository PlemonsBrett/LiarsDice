==========================
Database Schema Design
==========================

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
========

The LiarsDice game engine includes a comprehensive database schema designed to support game analytics, AI training data collection, and player behavior analysis. The schema is optimized for both real-time game operations and analytical queries.

Entity Relationship Diagram
============================

.. mermaid::

   erDiagram
       PLAYERS {
           INTEGER player_id PK
           VARCHAR username UK
           VARCHAR email
           TIMESTAMP created_date
           TIMESTAMP last_login
           INTEGER total_games
           INTEGER games_won
           REAL win_percentage
           TEXT preferences_json
       }

       GAMES {
           INTEGER game_id PK
           TIMESTAMP start_time
           TIMESTAMP end_time
           INTEGER total_players
           INTEGER winner_id FK
           INTEGER total_rounds
           INTEGER game_duration_seconds
           TEXT game_config_json
       }

       GAME_PARTICIPANTS {
           INTEGER participation_id PK
           INTEGER game_id FK
           INTEGER player_id FK
           VARCHAR player_type
           INTEGER final_position
           INTEGER rounds_survived
           INTEGER successful_guesses
           INTEGER successful_liar_calls
           INTEGER total_guesses
           INTEGER total_liar_calls
           REAL average_confidence
       }

       GAME_ROUNDS {
           INTEGER round_id PK
           INTEGER game_id FK
           INTEGER round_number
           INTEGER current_player_id FK
           INTEGER guess_count
           INTEGER guess_value
           BOOLEAN was_challenged
           BOOLEAN challenge_successful
           TEXT dice_state_json
           TIMESTAMP round_timestamp
           INTEGER decision_time_ms
       }

       AI_DECISIONS {
           INTEGER decision_id PK
           INTEGER game_id FK
           INTEGER round_id FK
           VARCHAR ai_level
           VARCHAR decision_type
           REAL confidence_score
           REAL calculated_probability
           INTEGER decision_time_ms
           BOOLEAN was_optimal
           TEXT decision_factors_json
           TIMESTAMP created_at
       }

       PLAYER_BEHAVIOR_PATTERNS {
           INTEGER pattern_id PK
           INTEGER player_id FK
           REAL bluff_frequency
           REAL conservativeness_score
           REAL adaptability_score
           INTEGER games_analyzed
           TIMESTAMP last_updated
           TEXT behavior_vector_json
       }

       TRAINING_DATA_EXPORTS {
           INTEGER export_id PK
           TIMESTAMP export_date
           VARCHAR export_format
           INTEGER games_included
           INTEGER rounds_included
           VARCHAR file_path
           TEXT metadata_json
       }

       AI_PERFORMANCE_METRICS {
           INTEGER metric_id PK
           VARCHAR ai_level
           TIMESTAMP measurement_date
           REAL win_rate
           REAL average_confidence
           REAL prediction_accuracy
           INTEGER total_decisions
           TEXT performance_details_json
       }

       %% Relationships
       PLAYERS ||--o{ GAMES : "wins"
       PLAYERS ||--o{ GAME_PARTICIPANTS : "participates"
       PLAYERS ||--o{ GAME_ROUNDS : "plays_round"
       PLAYERS ||--o{ PLAYER_BEHAVIOR_PATTERNS : "has_pattern"
       
       GAMES ||--o{ GAME_PARTICIPANTS : "includes"
       GAMES ||--o{ GAME_ROUNDS : "contains"
       GAMES ||--o{ AI_DECISIONS : "generates"
       
       GAME_ROUNDS ||--o{ AI_DECISIONS : "triggers"
       
       %% Indexes and Constraints
       GAMES ||--o{ TRAINING_DATA_EXPORTS : "exports_from"

Table Descriptions
==================

Core Game Tables
----------------

PLAYERS
~~~~~~~

Stores player account information and aggregate statistics.

.. list-table:: PLAYERS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - player_id
     - INTEGER
     - PRIMARY KEY
     - Unique player identifier
   * - username
     - VARCHAR
     - UNIQUE, NOT NULL
     - Player's chosen username
   * - email
     - VARCHAR
     - UNIQUE
     - Player's email address
   * - created_date
     - TIMESTAMP
     - NOT NULL
     - Account creation timestamp
   * - last_login
     - TIMESTAMP
     - 
     - Most recent login time
   * - total_games
     - INTEGER
     - DEFAULT 0
     - Total games played
   * - games_won
     - INTEGER
     - DEFAULT 0
     - Number of games won
   * - win_percentage
     - REAL
     - 
     - Calculated win rate (0.0-1.0)
   * - preferences_json
     - TEXT
     - 
     - User preferences in JSON format

**Indexes:**
- Primary: ``player_id``
- Unique: ``username``, ``email``
- Performance: ``last_login``, ``total_games``

GAMES
~~~~~

Records individual game sessions and metadata.

.. list-table:: GAMES Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - game_id
     - INTEGER
     - PRIMARY KEY
     - Unique game identifier
   * - start_time
     - TIMESTAMP
     - NOT NULL
     - Game start timestamp
   * - end_time
     - TIMESTAMP
     - 
     - Game completion timestamp
   * - total_players
     - INTEGER
     - NOT NULL
     - Number of participants
   * - winner_id
     - INTEGER
     - FOREIGN KEY
     - Reference to winning player
   * - total_rounds
     - INTEGER
     - 
     - Number of rounds played
   * - game_duration_seconds
     - INTEGER
     - 
     - Total game duration
   * - game_config_json
     - TEXT
     - 
     - Game configuration settings

**Relationships:**
- ``winner_id`` → ``PLAYERS.player_id``

**Indexes:**
- Primary: ``game_id``
- Foreign: ``winner_id``
- Performance: ``start_time``, ``total_players``

GAME_PARTICIPANTS
~~~~~~~~~~~~~~~~~

Links players to games with participation statistics.

.. list-table:: GAME_PARTICIPANTS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - participation_id
     - INTEGER
     - PRIMARY KEY
     - Unique participation record
   * - game_id
     - INTEGER
     - FOREIGN KEY
     - Reference to game
   * - player_id
     - INTEGER
     - FOREIGN KEY
     - Reference to player
   * - player_type
     - VARCHAR
     - NOT NULL
     - 'human', 'ai_beginner', 'ai_expert'
   * - final_position
     - INTEGER
     - 
     - Finishing rank (1st, 2nd, etc.)
   * - rounds_survived
     - INTEGER
     - 
     - Rounds before elimination
   * - successful_guesses
     - INTEGER
     - DEFAULT 0
     - Number of accurate guesses
   * - successful_liar_calls
     - INTEGER
     - DEFAULT 0
     - Number of correct liar calls
   * - total_guesses
     - INTEGER
     - DEFAULT 0
     - Total guesses made
   * - total_liar_calls
     - INTEGER
     - DEFAULT 0
     - Total liar calls made
   * - average_confidence
     - REAL
     - 
     - Average decision confidence

**Relationships:**
- ``game_id`` → ``GAMES.game_id``
- ``player_id`` → ``PLAYERS.player_id``

**Indexes:**
- Primary: ``participation_id``
- Composite: ``(game_id, player_id)``
- Performance: ``player_type``, ``final_position``

Detailed Game Data
------------------

GAME_ROUNDS
~~~~~~~~~~~

Captures individual round actions and outcomes.

.. list-table:: GAME_ROUNDS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - round_id
     - INTEGER
     - PRIMARY KEY
     - Unique round identifier
   * - game_id
     - INTEGER
     - FOREIGN KEY
     - Reference to parent game
   * - round_number
     - INTEGER
     - NOT NULL
     - Sequential round number
   * - current_player_id
     - INTEGER
     - FOREIGN KEY
     - Active player for this round
   * - guess_count
     - INTEGER
     - 
     - Number of dice guessed
   * - guess_value
     - INTEGER
     - 
     - Face value guessed (1-6)
   * - was_challenged
     - BOOLEAN
     - DEFAULT FALSE
     - Whether guess was challenged
   * - challenge_successful
     - BOOLEAN
     - 
     - Whether challenge was correct
   * - dice_state_json
     - TEXT
     - 
     - Complete dice state snapshot
   * - round_timestamp
     - TIMESTAMP
     - NOT NULL
     - Round completion time
   * - decision_time_ms
     - INTEGER
     - 
     - Time taken for decision

**Relationships:**
- ``game_id`` → ``GAMES.game_id``
- ``current_player_id`` → ``PLAYERS.player_id``

**Indexes:**
- Primary: ``round_id``
- Composite: ``(game_id, round_number)``
- Performance: ``current_player_id``, ``was_challenged``

AI and Analytics Tables
-----------------------

AI_DECISIONS
~~~~~~~~~~~~

Records AI decision-making process for analysis and training.

.. list-table:: AI_DECISIONS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - decision_id
     - INTEGER
     - PRIMARY KEY
     - Unique decision identifier
   * - game_id
     - INTEGER
     - FOREIGN KEY
     - Reference to game
   * - round_id
     - INTEGER
     - FOREIGN KEY
     - Reference to specific round
   * - ai_level
     - VARCHAR
     - NOT NULL
     - 'beginner', 'intermediate', 'expert'
   * - decision_type
     - VARCHAR
     - NOT NULL
     - 'guess', 'liar_call', 'pass'
   * - confidence_score
     - REAL
     - 
     - AI confidence (0.0-1.0)
   * - calculated_probability
     - REAL
     - 
     - Computed probability of success
   * - decision_time_ms
     - INTEGER
     - 
     - Processing time required
   * - was_optimal
     - BOOLEAN
     - 
     - Post-game analysis result
   * - decision_factors_json
     - TEXT
     - 
     - Detailed reasoning factors
   * - created_at
     - TIMESTAMP
     - NOT NULL
     - Decision timestamp

**Relationships:**
- ``game_id`` → ``GAMES.game_id``
- ``round_id`` → ``GAME_ROUNDS.round_id``

**Indexes:**
- Primary: ``decision_id``
- Performance: ``ai_level``, ``decision_type``, ``was_optimal``

PLAYER_BEHAVIOR_PATTERNS
~~~~~~~~~~~~~~~~~~~~~~~~~

Analyzes and stores player behavioral patterns for AI training.

.. list-table:: PLAYER_BEHAVIOR_PATTERNS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - pattern_id
     - INTEGER
     - PRIMARY KEY
     - Unique pattern identifier
   * - player_id
     - INTEGER
     - FOREIGN KEY
     - Reference to player
   * - bluff_frequency
     - REAL
     - 
     - How often player bluffs (0.0-1.0)
   * - conservativeness_score
     - REAL
     - 
     - Risk aversion measure
   * - adaptability_score
     - REAL
     - 
     - Strategy change frequency
   * - games_analyzed
     - INTEGER
     - 
     - Sample size for patterns
   * - last_updated
     - TIMESTAMP
     - 
     - Most recent analysis
   * - behavior_vector_json
     - TEXT
     - 
     - Detailed behavioral metrics

**Relationships:**
- ``player_id`` → ``PLAYERS.player_id``

**Indexes:**
- Primary: ``pattern_id``
- Unique: ``player_id``
- Performance: ``last_updated``, ``games_analyzed``

Data Export and Metrics
-----------------------

TRAINING_DATA_EXPORTS
~~~~~~~~~~~~~~~~~~~~~

Tracks machine learning dataset exports for AI training.

.. list-table:: TRAINING_DATA_EXPORTS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - export_id
     - INTEGER
     - PRIMARY KEY
     - Unique export identifier
   * - export_date
     - TIMESTAMP
     - NOT NULL
     - Export creation timestamp
   * - export_format
     - VARCHAR
     - NOT NULL
     - 'csv', 'json', 'parquet'
   * - games_included
     - INTEGER
     - 
     - Number of games in export
   * - rounds_included
     - INTEGER
     - 
     - Number of rounds in export
   * - file_path
     - VARCHAR
     - 
     - Storage location path
   * - metadata_json
     - TEXT
     - 
     - Export configuration metadata

**Indexes:**
- Primary: ``export_id``
- Performance: ``export_date``, ``export_format``

AI_PERFORMANCE_METRICS
~~~~~~~~~~~~~~~~~~~~~~~

Aggregated performance metrics for different AI difficulty levels.

.. list-table:: AI_PERFORMANCE_METRICS Table Schema
   :header-rows: 1
   :widths: 20 15 15 50

   * - Column
     - Type
     - Constraints
     - Description
   * - metric_id
     - INTEGER
     - PRIMARY KEY
     - Unique metric identifier
   * - ai_level
     - VARCHAR
     - NOT NULL
     - AI difficulty level
   * - measurement_date
     - TIMESTAMP
     - NOT NULL
     - Measurement timestamp
   * - win_rate
     - REAL
     - 
     - Overall win percentage
   * - average_confidence
     - REAL
     - 
     - Mean confidence score
   * - prediction_accuracy
     - REAL
     - 
     - Accuracy of probability estimates
   * - total_decisions
     - INTEGER
     - 
     - Sample size for metrics
   * - performance_details_json
     - TEXT
     - 
     - Detailed performance breakdown

**Indexes:**
- Primary: ``metric_id``
- Composite: ``(ai_level, measurement_date)``
- Performance: ``measurement_date``

Database Design Principles
===========================

Normalization
--------------

The schema follows **Third Normal Form (3NF)** principles:

- **1NF**: All columns contain atomic values
- **2NF**: No partial dependencies on composite keys
- **3NF**: No transitive dependencies

This ensures data integrity while maintaining query performance.

Indexing Strategy
-----------------

**Primary Indexes:**
- All tables have clustered primary key indexes
- Foreign key columns are indexed for join performance

**Performance Indexes:**
- Timestamp columns for temporal queries
- Frequently filtered columns (player_type, ai_level)
- Composite indexes for common query patterns

**Unique Constraints:**
- Business logic constraints (username, email)
- Data integrity constraints (player-game participation)

JSON Storage
------------

Several tables use JSON columns for flexible schema evolution:

- **game_config_json**: Game rule variations and settings
- **preferences_json**: User-specific configuration
- **dice_state_json**: Complete game state snapshots
- **decision_factors_json**: AI reasoning details
- **behavior_vector_json**: Multi-dimensional behavioral data

This approach balances structure with flexibility for evolving requirements.

Query Patterns
===============

Common Analytics Queries
-------------------------

**Player Performance Analysis:**

.. code-block:: sql

   SELECT 
       p.username,
       p.total_games,
       p.win_percentage,
       AVG(gp.successful_guesses::REAL / gp.total_guesses) as guess_accuracy
   FROM players p
   JOIN game_participants gp ON p.player_id = gp.player_id
   WHERE p.total_games >= 10
   GROUP BY p.player_id, p.username, p.total_games, p.win_percentage
   ORDER BY p.win_percentage DESC;

**AI Performance Comparison:**

.. code-block:: sql

   SELECT 
       ai_level,
       AVG(win_rate) as avg_win_rate,
       AVG(prediction_accuracy) as avg_accuracy,
       SUM(total_decisions) as total_decisions
   FROM ai_performance_metrics
   WHERE measurement_date >= NOW() - INTERVAL '30 days'
   GROUP BY ai_level
   ORDER BY avg_win_rate DESC;

**Game Session Analysis:**

.. code-block:: sql

   SELECT 
       g.game_id,
       g.total_players,
       g.total_rounds,
       COUNT(gr.round_id) as recorded_rounds,
       AVG(gr.decision_time_ms) as avg_decision_time
   FROM games g
   JOIN game_rounds gr ON g.game_id = gr.game_id
   WHERE g.start_time >= NOW() - INTERVAL '7 days'
   GROUP BY g.game_id, g.total_players, g.total_rounds
   HAVING COUNT(gr.round_id) = g.total_rounds;

Maintenance and Optimization
=============================

Data Retention Policy
----------------------

- **Game data**: Retain for 2 years for trend analysis
- **AI decisions**: Retain indefinitely for training
- **Performance metrics**: Aggregate monthly, retain raw data for 6 months
- **Player patterns**: Update weekly, retain history for comparison

Backup Strategy
---------------

- **Daily incremental backups** for transactional data
- **Weekly full backups** with compression
- **Monthly archival** to long-term storage
- **Cross-region replication** for disaster recovery

Performance Monitoring
----------------------

Key metrics to monitor:

- Query response times for analytics queries
- Index usage statistics
- Table growth rates
- Concurrent user load impact

.. seealso::
   - :doc:`../architecture/overview` - System architecture overview
   - :doc:`uml-diagrams` - Class structure diagrams  
   - :doc:`ai-enhancements` - AI implementation details