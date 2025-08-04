#include <liarsdice/database/database_manager.hpp>
#include <liarsdice/database/database_config.hpp>
#include <liarsdice/database/database_connection.hpp>
#include <liarsdice/database/connection_manager.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <iomanip>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

void print_menu() {
    std::cout << "\n=== Liar's Dice Database Demo ===\n";
    std::cout << "1. Create tables\n";
    std::cout << "2. Insert player\n";
    std::cout << "3. List all players\n";
    std::cout << "4. Update player score\n";
    std::cout << "5. Delete player\n";
    std::cout << "6. Show table schema\n";
    std::cout << "7. Run transaction test\n";
    std::cout << "0. Exit\n";
    std::cout << "Choice: ";
}

void create_tables(DatabaseManager& db) {
    std::cout << "\nCreating tables...\n";
    
    auto result = db.execute(R"(
        CREATE TABLE IF NOT EXISTS players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            total_games INTEGER DEFAULT 0,
            games_won INTEGER DEFAULT 0,
            games_lost INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    if (result) {
        std::cout << "✓ Players table created\n";
    } else {
        std::cout << "✗ Error: " << result.error().full_message() << "\n";
        return;
    }
    
    result = db.execute(R"(
        CREATE TABLE IF NOT EXISTS game_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id INTEGER,
            opponent_name TEXT,
            won BOOLEAN,
            rounds_played INTEGER,
            played_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (player_id) REFERENCES players(id)
        )
    )");
    
    if (result) {
        std::cout << "✓ Game history table created\n";
    } else {
        std::cout << "✗ Error: " << result.error().full_message() << "\n";
    }
}

void insert_player(DatabaseManager& db) {
    std::string name;
    std::cout << "\nEnter player name: ";
    std::getline(std::cin, name);
    
    if (name.empty()) {
        std::cout << "Name cannot be empty\n";
        return;
    }
    
    auto stmt = db.prepare("INSERT INTO players (name) VALUES (?)");
    if (!stmt) {
        std::cout << "✗ Error preparing statement: " << stmt.error().full_message() << "\n";
        return;
    }
    
    stmt.value()->bind(1, name);
    auto result = db.execute_prepared(stmt.value());
    
    if (result) {
        std::cout << "✓ Player '" << name << "' added successfully\n";
    } else {
        std::cout << "✗ Error: " << result.error().full_message() << "\n";
    }
}

void list_players(DatabaseManager& db) {
    std::cout << "\n=== All Players ===\n";
    
    auto stmt = db.prepare(R"(
        SELECT id, name, total_games, games_won, games_lost, 
               datetime(created_at, 'localtime') as created
        FROM players
        ORDER BY name
    )");
    
    if (!stmt) {
        std::cout << "✗ Error: " << stmt.error().full_message() << "\n";
        return;
    }
    
    std::cout << std::setw(5) << "ID" 
              << std::setw(20) << "Name"
              << std::setw(10) << "Games"
              << std::setw(10) << "Won"
              << std::setw(10) << "Lost"
              << std::setw(20) << "Created" << "\n";
    std::cout << std::string(75, '-') << "\n";
    
    int count = 0;
    db.execute_prepared(stmt.value(), [&count](const PreparedStatement& row) {
        auto id = std::get<int64_t>(row.get_column(0));
        auto name = std::get<std::string>(row.get_column(1));
        auto total = std::get<int64_t>(row.get_column(2));
        auto won = std::get<int64_t>(row.get_column(3));
        auto lost = std::get<int64_t>(row.get_column(4));
        auto created = std::get<std::string>(row.get_column(5));
        
        std::cout << std::setw(5) << id
                  << std::setw(20) << name
                  << std::setw(10) << total
                  << std::setw(10) << won
                  << std::setw(10) << lost
                  << std::setw(20) << created << "\n";
        count++;
    });
    
    std::cout << "\nTotal players: " << count << "\n";
}

void update_player_score(DatabaseManager& db) {
    std::cout << "\nEnter player ID: ";
    int player_id;
    std::cin >> player_id;
    std::cin.ignore();
    
    std::cout << "Did player win? (y/n): ";
    char won_char;
    std::cin >> won_char;
    std::cin.ignore();
    
    bool won = (won_char == 'y' || won_char == 'Y');
    
    // Use transaction to update both tables
    auto result = db.with_transaction([player_id, won](DatabaseManager& mgr) {
        // Update player stats
        auto update_stmt = mgr.prepare(R"(
            UPDATE players 
            SET total_games = total_games + 1,
                games_won = games_won + ?,
                games_lost = games_lost + ?
            WHERE id = ?
        )");
        
        if (!update_stmt) {
            return DatabaseResult<void>(update_stmt.error());
        }
        
        update_stmt.value()->bind(1, won ? 1 : 0);
        update_stmt.value()->bind(2, won ? 0 : 1);
        update_stmt.value()->bind(3, player_id);
        
        auto exec_result = mgr.execute_prepared(update_stmt.value());
        if (!exec_result) {
            return DatabaseResult<void>(exec_result.error());
        }
        
        // Add game history
        auto history_stmt = mgr.prepare(R"(
            INSERT INTO game_history (player_id, opponent_name, won, rounds_played)
            VALUES (?, 'Demo Opponent', ?, ?)
        )");
        
        if (!history_stmt) {
            return DatabaseResult<void>(history_stmt.error());
        }
        
        history_stmt.value()->bind(1, player_id);
        history_stmt.value()->bind(2, won);
        history_stmt.value()->bind(3, 5); // Demo rounds
        
        exec_result = mgr.execute_prepared(history_stmt.value());
        if (!exec_result) {
            return DatabaseResult<void>(exec_result.error());
        }
        
        return DatabaseResult<void>();
    });
    
    if (result) {
        std::cout << "✓ Player score updated successfully\n";
    } else {
        std::cout << "✗ Error: " << result.error().full_message() << "\n";
    }
}

void delete_player(DatabaseManager& db) {
    std::cout << "\nEnter player ID to delete: ";
    int player_id;
    std::cin >> player_id;
    std::cin.ignore();
    
    auto result = db.with_transaction([player_id](DatabaseManager& mgr) {
        // Delete game history first (foreign key constraint)
        auto del_history = mgr.prepare("DELETE FROM game_history WHERE player_id = ?");
        if (!del_history) {
            return DatabaseResult<void>(del_history.error());
        }
        
        del_history.value()->bind(1, player_id);
        auto exec_result = mgr.execute_prepared(del_history.value());
        if (!exec_result) {
            return DatabaseResult<void>(exec_result.error());
        }
        
        // Delete player
        auto del_player = mgr.prepare("DELETE FROM players WHERE id = ?");
        if (!del_player) {
            return DatabaseResult<void>(del_player.error());
        }
        
        del_player.value()->bind(1, player_id);
        exec_result = mgr.execute_prepared(del_player.value());
        if (!exec_result) {
            return DatabaseResult<void>(exec_result.error());
        }
        
        return DatabaseResult<void>();
    });
    
    if (result) {
        std::cout << "✓ Player deleted successfully\n";
    } else {
        std::cout << "✗ Error: " << result.error().full_message() << "\n";
    }
}

void show_schema(DatabaseManager& db) {
    std::cout << "\n=== Database Schema ===\n";
    
    auto stmt = db.prepare(R"(
        SELECT name, sql 
        FROM sqlite_master 
        WHERE type='table' AND name NOT LIKE 'sqlite_%'
        ORDER BY name
    )");
    
    if (!stmt) {
        std::cout << "✗ Error: " << stmt.error().full_message() << "\n";
        return;
    }
    
    db.execute_prepared(stmt.value(), [](const PreparedStatement& row) {
        auto name = std::get<std::string>(row.get_column(0));
        auto sql = std::get<std::string>(row.get_column(1));
        
        std::cout << "\nTable: " << name << "\n";
        std::cout << sql << "\n";
    });
}

void run_transaction_test(DatabaseManager& db) {
    std::cout << "\n=== Transaction Test ===\n";
    std::cout << "This will attempt to insert two players in a transaction.\n";
    std::cout << "The second insert will fail due to duplicate name constraint.\n\n";
    
    auto result = db.with_transaction([](DatabaseManager& mgr) {
        std::cout << "1. Inserting 'Transaction Test 1'...\n";
        auto stmt1 = mgr.prepare("INSERT INTO players (name) VALUES (?)");
        if (!stmt1) return DatabaseResult<void>(stmt1.error());
        
        stmt1.value()->bind(1, std::string("Transaction Test 1"));
        auto res1 = mgr.execute_prepared(stmt1.value());
        if (!res1) return DatabaseResult<void>(res1.error());
        std::cout << "   ✓ Success\n";
        
        std::cout << "2. Inserting 'Transaction Test 1' again (should fail)...\n";
        auto stmt2 = mgr.prepare("INSERT INTO players (name) VALUES (?)");
        if (!stmt2) return DatabaseResult<void>(stmt2.error());
        
        stmt2.value()->bind(1, std::string("Transaction Test 1"));
        auto res2 = mgr.execute_prepared(stmt2.value());
        if (!res2) {
            std::cout << "   ✗ Failed as expected: " << res2.error().message() << "\n";
            return DatabaseResult<void>(res2.error());
        }
        
        return DatabaseResult<void>();
    });
    
    if (result) {
        std::cout << "\nTransaction committed successfully (unexpected)\n";
    } else {
        std::cout << "\nTransaction rolled back as expected\n";
        std::cout << "Error: " << result.error().full_message() << "\n";
    }
    
    // Verify rollback
    std::cout << "\nVerifying rollback...\n";
    auto check = db.prepare("SELECT COUNT(*) FROM players WHERE name LIKE 'Transaction Test%'");
    if (check) {
        int count = 0;
        db.execute_prepared(check.value(), [&count](const PreparedStatement& row) {
            count = static_cast<int>(std::get<int64_t>(row.get_column(0)));
        });
        std::cout << "Number of 'Transaction Test' players: " << count << " (should be 0)\n";
    }
}

int main() {
    try {
        // Set up database directory
        auto db_dir = fs::current_path() / "demo_database";
        if (!fs::exists(db_dir)) {
            fs::create_directories(db_dir);
        }
        
        // Configure database
        auto& config = DatabaseConfig::instance();
        config.set_database_directory(db_dir);
        
        std::cout << "Database location: " << config.get_database_file_path() << "\n";
        
        // Initialize ConnectionManager with pool configuration
        auto& conn_mgr = ConnectionManager::instance();
        if (!conn_mgr.is_initialized()) {
            ConnectionPool::PoolConfig pool_config;
            pool_config.min_connections = 2;
            pool_config.max_connections = 5;
            pool_config.enable_health_checks = true;
            pool_config.health_check_interval = std::chrono::seconds(30);
            
            std::cout << "Initializing ConnectionManager...\n";
            conn_mgr.configure(pool_config);
            std::cout << "ConnectionManager initialized successfully\n";
        }
        
        // Create database manager
        DatabaseManager db_mgr;
        
        // Main loop
        while (true) {
            print_menu();
            
            int choice;
            std::cin >> choice;
            std::cin.ignore(); // Clear newline
            
            switch (choice) {
                case 1:
                    create_tables(db_mgr);
                    break;
                case 2:
                    insert_player(db_mgr);
                    break;
                case 3:
                    list_players(db_mgr);
                    break;
                case 4:
                    update_player_score(db_mgr);
                    break;
                case 5:
                    delete_player(db_mgr);
                    break;
                case 6:
                    show_schema(db_mgr);
                    break;
                case 7:
                    run_transaction_test(db_mgr);
                    break;
                case 0:
                    std::cout << "\nShutting down...\n";
                    conn_mgr.shutdown();
                    std::cout << "Goodbye!\n";
                    return 0;
                default:
                    std::cout << "\nInvalid choice. Please try again.\n";
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}