-- Add statistics tracking tables
-- Version: 2
-- Description: Add detailed statistics and analytics tables

-- Round-by-round game details
CREATE TABLE IF NOT EXISTS game_rounds (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    game_id INTEGER NOT NULL,
    round_number INTEGER NOT NULL,
    starting_player_id INTEGER NOT NULL,
    ending_player_id INTEGER,
    total_dice INTEGER NOT NULL,
    final_guess_count INTEGER,
    final_guess_face INTEGER,
    actual_count INTEGER,
    liar_called BOOLEAN DEFAULT 0,
    duration_seconds INTEGER,
    FOREIGN KEY (game_id) REFERENCES game_history(id),
    FOREIGN KEY (starting_player_id) REFERENCES players(id),
    FOREIGN KEY (ending_player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_game_rounds_game ON game_rounds(game_id);

-- Individual player actions
CREATE TABLE IF NOT EXISTS player_actions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    game_id INTEGER NOT NULL,
    round_number INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    action_type TEXT NOT NULL, -- 'guess', 'call_liar'
    action_timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    guess_count INTEGER,
    guess_face INTEGER,
    was_bluff BOOLEAN,
    was_correct BOOLEAN,
    FOREIGN KEY (game_id) REFERENCES game_history(id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_player_actions_game ON player_actions(game_id, round_number);
CREATE INDEX IF NOT EXISTS idx_player_actions_player ON player_actions(player_id);

-- Player statistics summary table
CREATE TABLE IF NOT EXISTS player_statistics (
    player_id INTEGER PRIMARY KEY,
    avg_game_duration INTEGER DEFAULT 0,
    avg_rounds_per_game REAL DEFAULT 0,
    win_rate REAL DEFAULT 0,
    favorite_dice_face INTEGER,
    bluff_success_rate REAL DEFAULT 0,
    call_accuracy_rate REAL DEFAULT 0,
    most_played_hour INTEGER,
    streak_current INTEGER DEFAULT 0,
    streak_best INTEGER DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (player_id) REFERENCES players(id)
);

-- Leaderboard snapshots
CREATE TABLE IF NOT EXISTS leaderboard_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    snapshot_date DATE NOT NULL,
    rank INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    rating INTEGER NOT NULL,
    games_played INTEGER NOT NULL,
    win_rate REAL NOT NULL,
    UNIQUE(snapshot_date, rank),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_leaderboard_date ON leaderboard_history(snapshot_date);

-- Create triggers to maintain statistics
CREATE TRIGGER IF NOT EXISTS update_player_statistics_on_game_end
    AFTER UPDATE OF is_completed ON game_history
    WHEN NEW.is_completed = 1
BEGIN
    -- Update player statistics for all participants
    INSERT OR REPLACE INTO player_statistics (player_id)
    SELECT DISTINCT player_id FROM game_participants WHERE game_id = NEW.id;
END;