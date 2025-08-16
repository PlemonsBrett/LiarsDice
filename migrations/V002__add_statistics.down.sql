-- Rollback statistics tables
-- Version: 2

DROP TRIGGER IF EXISTS update_player_statistics_on_game_end;
DROP TABLE IF EXISTS leaderboard_history;
DROP TABLE IF EXISTS player_statistics;
DROP TABLE IF EXISTS player_actions;
DROP TABLE IF EXISTS game_rounds;