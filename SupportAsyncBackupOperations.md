# Supporting Async Backup Operations

## Overview
The current backup implementation in `BackupManager` is synchronous, which can cause I/O blocking during large database backups. This document outlines the steps needed to implement asynchronous backup operations using Boost.Asio.

## Current Implementation Issues
1. **Blocking I/O**: `copy_database_file()` and `compress_backup()` block the calling thread
2. **No progress reporting**: Large backups provide no feedback during operation
3. **No cancellation**: Once started, backups cannot be cancelled
4. **Thread blocking**: Long-running backups can block the connection pool thread

## Proposed Async Architecture

### 1. Async Backup Service
Create a dedicated `AsyncBackupService` class that manages backup operations on a separate thread pool:

```cpp
class AsyncBackupService {
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::vector<std::thread> thread_pool_;
    
public:
    // Returns a future for the backup operation
    std::future<DatabaseResult<BackupInfo>> create_backup_async(
        const std::string& backup_name,
        std::function<void(size_t bytes_copied, size_t total_bytes)> progress_callback = nullptr
    );
    
    // Cancel a running backup
    void cancel_backup(const std::string& backup_id);
};
```

### 2. Async File Operations
Replace blocking file operations with async versions:

```cpp
// Current blocking implementation
boost::filesystem::copy_file(source, destination);

// Proposed async implementation using Boost.Asio
boost::asio::async_read(source_file, buffer,
    [](boost::system::error_code ec, std::size_t bytes_transferred) {
        // Handle chunk read
    });

boost::asio::async_write(dest_file, buffer,
    [](boost::system::error_code ec, std::size_t bytes_transferred) {
        // Handle chunk write
    });
```

### 3. Progress Reporting
Implement progress callbacks for long-running operations:

```cpp
struct BackupProgress {
    std::string backup_id;
    std::atomic<size_t> bytes_processed;
    std::atomic<size_t> total_bytes;
    std::atomic<bool> is_compressing;
    std::atomic<bool> is_cancelled;
    
    double get_percentage() const {
        return (double)bytes_processed / total_bytes * 100.0;
    }
};
```

### 4. Integration Points

#### Modify BackupManager
```cpp
class BackupManager {
    // Add async versions of existing methods
    std::future<DatabaseResult<BackupInfo>> create_backup_async(
        const std::string& backup_name = ""
    );
    
    // Get progress of running backups
    std::vector<BackupProgress> get_running_backups() const;
    
    // Cancel a backup
    bool cancel_backup(const std::string& backup_id);
    
private:
    std::unique_ptr<AsyncBackupService> async_service_;
    std::map<std::string, BackupProgress> running_backups_;
};
```

#### Update ConnectionManager
```cpp
// Add async backup execution that doesn't block connection pool
template<typename CompletionHandler>
void execute_backup_async(
    const std::string& backup_name,
    CompletionHandler&& handler
) {
    boost::asio::post(io_context_, [=]() {
        // Perform backup on io_context thread pool
        auto result = backup_manager_->create_backup_async(backup_name);
        handler(result);
    });
}
```

## Implementation Steps

### Phase 1: Infrastructure (2-3 days)
1. Create `AsyncBackupService` class with thread pool
2. Set up Boost.Asio io_context and work guards
3. Implement backup job queue and scheduling

### Phase 2: Async File Operations (3-4 days)
1. Implement chunked async file reading/writing
2. Add progress tracking for file operations
3. Implement cancellation tokens for operations
4. Add error recovery and retry logic

### Phase 3: Compression (2 days)
1. Make compression async using Boost.Iostreams with async filters
2. Add progress reporting for compression
3. Implement streaming compression to avoid memory spikes

### Phase 4: Integration (2-3 days)
1. Update `BackupManager` to support both sync and async operations
2. Add async methods to `ConnectionManager`
3. Update database demo to showcase async backups
4. Add comprehensive tests for async operations

### Phase 5: Optimization (1-2 days)
1. Tune thread pool size based on system resources
2. Implement adaptive chunk sizes for file operations
3. Add caching for frequently accessed backup metadata
4. Profile and optimize memory usage

## Dependencies Required

### Additional Boost Components
- `boost::asio::stream_file` (Boost 1.84+) for async file I/O
- `boost::process` for potential external compression tools
- `boost::fiber` (optional) for coroutine-based implementation

### Code Changes Required

#### CMakeLists.txt
```cmake
find_package(Boost REQUIRED COMPONENTS 
    # ... existing components ...
    fiber  # Optional for coroutines
)
```

#### include/liarsdice/database/async_backup_service.hpp
New file containing async backup implementation

#### source/liarsdice/database/async_backup_service.cpp
Implementation of async backup operations

## Testing Considerations

### Unit Tests
- Test async backup creation with various file sizes
- Test cancellation at different stages
- Test progress reporting accuracy
- Test error handling and recovery
- Test concurrent backup operations

### Performance Tests
- Measure I/O throughput improvement
- Test CPU usage during async operations
- Measure memory usage patterns
- Test system responsiveness during large backups

### Integration Tests
- Test interaction with connection pool
- Test transaction handling during backups
- Test backup restore from async-created backups
- Test cleanup of cancelled backups

## Alternative Approaches

### 1. Using std::async
Simpler but less control:
```cpp
auto future = std::async(std::launch::async, [this, backup_name]() {
    return create_backup(backup_name);
});
```

### 2. External Process
Use external tools like `sqlite3` CLI:
```cpp
boost::process::async_system(
    io_context_,
    "sqlite3 source.db .backup target.db",
    [](boost::system::error_code ec, int exit_code) {
        // Handle completion
    }
);
```

### 3. SQLite Online Backup API
Use SQLite's built-in async backup API (requires SQLite 3.27.0+):
```cpp
sqlite3_backup* backup = sqlite3_backup_init(dest_db, "main", source_db, "main");
// Step through backup in chunks
while (sqlite3_backup_step(backup, chunk_size) == SQLITE_OK) {
    // Report progress
    int remaining = sqlite3_backup_remaining(backup);
    int pagecount = sqlite3_backup_pagecount(backup);
}
sqlite3_backup_finish(backup);
```

## Risks and Mitigations

### Risk 1: Resource Exhaustion
**Mitigation**: Limit concurrent backup operations, implement resource pools

### Risk 2: Data Consistency
**Mitigation**: Use SQLite's backup API or ensure proper locking

### Risk 3: Increased Complexity
**Mitigation**: Keep sync API as default, async as opt-in feature

### Risk 4: Platform Compatibility
**Mitigation**: Provide platform-specific implementations where needed

## Estimated Effort
- **Total Development**: 10-15 days
- **Testing**: 3-5 days
- **Documentation**: 1-2 days
- **Code Review**: 1-2 days

## Benefits Once Implemented
1. **Non-blocking Operations**: UI/API remains responsive during backups
2. **Progress Monitoring**: Real-time backup progress information
3. **Cancellable Operations**: Ability to cancel long-running backups
4. **Better Resource Utilization**: Parallel I/O operations when possible
5. **Improved User Experience**: No application freezes during backups

## Next Steps
1. Review and approve design with team
2. Create feature branch `feature/async-backups`
3. Implement Phase 1 infrastructure
4. Iteratively implement remaining phases
5. Performance testing and optimization
6. Documentation and examples
7. Merge to main branch

## References
- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [SQLite Backup API](https://www.sqlite.org/backup.html)
- [Boost.Process for External Tools](https://www.boost.org/doc/libs/release/doc/html/process.html)
- [C++ Async Patterns](https://en.cppreference.com/w/cpp/thread/async)