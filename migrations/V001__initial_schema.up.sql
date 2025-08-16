-- Initial schema for Liar's Dice database
-- Version: 1
-- Description: Create initial tables for player management

CREATE TABLE IF NOT EXISTS players (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL,
    email TEXT UNIQUE,
    total_games INTEGER DEFAULT 0,
    games_won INTEGER DEFAULT 0,
    games_lost INTEGER DEFAULT 0,
    total_points INTEGER DEFAULT 0,
    highest_score INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_active BOOLEAN DEFAULT 1
);

CREATE INDEX IF NOT EXISTS idx_players_username ON players(username);
CREATE INDEX IF NOT EXISTS idx_players_email ON players(email);
CREATE INDEX IF NOT EXISTS idx_players_last_login ON players(last_login);

-- Game history table
CREATE TABLE IF NOT EXISTS game_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    game_uuid TEXT NOT NULL UNIQUE,
    started_at TIMESTAMP NOT NULL,
    ended_at TIMESTAMP,
    total_players INTEGER NOT NULL,
    total_rounds INTEGER DEFAULT 0,
    winner_id INTEGER,
    game_mode TEXT DEFAULT 'classic',
    is_completed BOOLEAN DEFAULT 0,
    FOREIGN KEY (winner_id) REFERENCES players(id)
);

CREATE INDEX IF NOT EXISTS idx_game_history_uuid ON game_history(game_uuid);
CREATE INDEX IF NOT EXISTS idx_game_history_dates ON game_history(started_at, ended_at);

-- Player participation in games
CREATE TABLE IF NOT EXISTS game_participants (
    game_id INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    starting_position INTEGER NOT NULL,
    final_position INTEGER,
    points_scored INTEGER DEFAULT 0,
    rounds_survived INTEGER DEFAULT 0,
    total_guesses INTEGER DEFAULT 0,
    successful_calls INTEGER DEFAULT 0,
    bluffs_attempted INTEGER DEFAULT 0,
    bluffs_caught INTEGER DEFAULT 0,
    PRIMARY KEY (game_id, player_id),
    FOREIGN KEY (game_id) REFERENCES game_history(id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

-- Achievement definitions
CREATE TABLE IF NOT EXISTS achievements (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    description TEXT,
    points INTEGER DEFAULT 0,
    icon_path TEXT,
    category TEXT DEFAULT 'general'
);

-- Player achievements
CREATE TABLE IF NOT EXISTS player_achievements (
    player_id INTEGER NOT NULL,
    achievement_id INTEGER NOT NULL,
    earned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    game_id INTEGER,
    PRIMARY KEY (player_id, achievement_id),
    FOREIGN KEY (player_id) REFERENCES players(id),
    FOREIGN KEY (achievement_id) REFERENCES achievements(id),
    FOREIGN KEY (game_id) REFERENCES game_history(id)
);

-- Insert default achievements
INSERT OR IGNORE INTO achievements (code, name, description, points, category) VALUES
    ('first_win', 'First Victory', 'Win your first game', 10, 'milestone'),
    ('win_streak_5', 'Hot Streak', 'Win 5 games in a row', 50, 'skill'),
    ('perfect_game', 'Flawless Victory', 'Win without losing a single point', 100, 'skill'),
    ('comeback_king', 'Comeback King', 'Win after being down to 1 point', 75, 'skill'),
    ('bluff_master', 'Bluff Master', 'Successfully bluff 10 times in one game', 25, 'strategy'),
    ('truth_seeker', 'Truth Seeker', 'Correctly call liar 10 times in one game', 25, 'strategy'),
    ('games_100', 'Centurion', 'Play 100 games', 50, 'milestone'),
    ('games_1000', 'Dedicated Player', 'Play 1000 games', 200, 'milestone');