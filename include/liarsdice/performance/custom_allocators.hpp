#pragma once

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/pool/pool.hpp>
#include <boost/align/aligned_allocator.hpp>
#include <memory>
#include <atomic>
#include <mutex>

namespace liarsdice::performance {

/**
 * @brief Fast pool allocator for small objects
 * 
 * Uses boost::pool for efficient allocation/deallocation
 * of small, fixed-size objects.
 * 
 * @tparam T Type to allocate
 * @tparam BlockSize Number of objects per block
 */
template<typename T, std::size_t BlockSize = 32>
class FastPoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    // Tag for singleton pool
    struct pool_tag {};
    
    using pool_type = boost::singleton_pool<pool_tag, sizeof(T), 
                                           boost::default_user_allocator_new_delete,
                                           boost::details::pool::null_mutex,
                                           BlockSize>;
    
    template<typename U>
    struct rebind {
        using other = FastPoolAllocator<U, BlockSize>;
    };
    
    FastPoolAllocator() noexcept = default;
    FastPoolAllocator(const FastPoolAllocator&) noexcept = default;
    
    template<typename U>
    FastPoolAllocator(const FastPoolAllocator<U, BlockSize>&) noexcept {}
    
    ~FastPoolAllocator() = default;
    
    pointer allocate(size_type n) {
        if (n == 1) {
            return static_cast<pointer>(pool_type::malloc());
        }
        // Fall back to standard allocation for arrays
        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }
    
    void deallocate(pointer p, size_type n) noexcept {
        if (n == 1) {
            pool_type::free(p);
        } else {
            ::operator delete(p);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) noexcept {
        p->~U();
    }
    
    bool operator==(const FastPoolAllocator&) const noexcept { return true; }
    bool operator!=(const FastPoolAllocator&) const noexcept { return false; }
    
    // Get pool statistics
    static std::size_t get_allocated_count() {
        return pool_type::get_requested_size() / sizeof(T);
    }
    
    static void release_memory() {
        pool_type::release_memory();
    }
    
    static void purge_memory() {
        pool_type::purge_memory();
    }
};

/**
 * @brief SIMD-aligned allocator for vectorized operations
 * 
 * Ensures memory alignment for optimal SIMD performance.
 * 
 * @tparam T Type to allocate
 * @tparam Alignment Alignment requirement (default: 32 bytes for AVX)
 */
template<typename T, std::size_t Alignment = 32>
using SimdAllocator = boost::alignment::aligned_allocator<T, Alignment>;

/**
 * @brief Object pool for game state objects
 * 
 * Pre-allocates and reuses game state objects to avoid
 * allocation overhead during gameplay.
 * 
 * @tparam T Object type
 */
template<typename T>
class GameObjectPool {
public:
    explicit GameObjectPool(std::size_t initial_size = 16)
        : pool_(initial_size) {}
    
    /**
     * @brief Acquire object from pool
     * @tparam Args Constructor arguments
     * @param args Arguments to forward to constructor
     * @return Unique pointer to object with custom deleter
     */
    template<typename... Args>
    std::unique_ptr<T, std::function<void(T*)>> acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        T* obj = pool_.construct(std::forward<Args>(args)...);
        ++allocated_count_;
        
        // Custom deleter returns object to pool
        auto deleter = [this](T* p) {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_.destroy(p);
            --allocated_count_;
        };
        
        return std::unique_ptr<T, std::function<void(T*)>>(obj, deleter);
    }
    
    /**
     * @brief Get number of allocated objects
     * @return Current allocation count
     */
    std::size_t allocated_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return allocated_count_;
    }
    
    /**
     * @brief Release unused memory
     */
    void release_memory() {
        std::lock_guard<std::mutex> lock(mutex_);
        // boost::object_pool doesn't provide direct memory release
        // Objects are released when pool is destroyed
    }
    
private:
    mutable std::mutex mutex_;
    boost::object_pool<T> pool_;
    std::size_t allocated_count_ = 0;
};

/**
 * @brief Memory arena for temporary allocations
 * 
 * Stack-based allocation pattern for temporary objects
 * with bulk deallocation.
 */
class MemoryArena {
public:
    explicit MemoryArena(std::size_t size = 1024 * 1024)  // 1MB default
        : buffer_(size), offset_(0) {}
    
    /**
     * @brief Allocate memory from arena
     * @param size Number of bytes
     * @param alignment Alignment requirement
     * @return Pointer to allocated memory
     */
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        // Align offset
        std::size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        
        if (aligned_offset + size > buffer_.size()) {
            throw std::bad_alloc();
        }
        
        void* ptr = buffer_.data() + aligned_offset;
        offset_ = aligned_offset + size;
        
        return ptr;
    }
    
    /**
     * @brief Allocate typed object
     * @tparam T Object type
     * @tparam Args Constructor arguments
     * @param args Arguments to forward
     * @return Pointer to constructed object
     */
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Reset arena to initial state
     */
    void reset() noexcept {
        offset_ = 0;
    }
    
    /**
     * @brief Get current memory usage
     * @return Bytes currently allocated
     */
    std::size_t used() const noexcept {
        return offset_;
    }
    
    /**
     * @brief Get total arena size
     * @return Total bytes available
     */
    std::size_t capacity() const noexcept {
        return buffer_.size();
    }
    
private:
    std::vector<char> buffer_;
    std::size_t offset_;
};

/**
 * @brief Specialized allocators for game components
 */
namespace allocators {

// Fast allocator for game states (8-byte objects)
using GameStateAllocator = FastPoolAllocator<char, 128>;

// SIMD-aligned allocator for dice arrays
using DiceArrayAllocator = SimdAllocator<unsigned int, 32>;

// Pool allocator for AI decision objects
template<typename T>
using AIDecisionAllocator = boost::pool_allocator<T>;

// Fast allocator for small strings
using StringAllocator = boost::pool_allocator<char>;

} // namespace allocators

/**
 * @brief Memory usage tracker
 * 
 * Provides statistics about memory allocation patterns.
 */
class MemoryTracker {
public:
    struct Stats {
        std::atomic<std::size_t> allocations{0};
        std::atomic<std::size_t> deallocations{0};
        std::atomic<std::size_t> bytes_allocated{0};
        std::atomic<std::size_t> bytes_deallocated{0};
        std::atomic<std::size_t> peak_usage{0};
        std::atomic<std::size_t> current_usage{0};
    };
    
    static MemoryTracker& instance() {
        static MemoryTracker tracker;
        return tracker;
    }
    
    void track_allocation(std::size_t size) {
        stats_.allocations++;
        stats_.bytes_allocated += size;
        stats_.current_usage += size;
        
        // Update peak usage
        std::size_t current = stats_.current_usage.load();
        std::size_t peak = stats_.peak_usage.load();
        while (current > peak && 
               !stats_.peak_usage.compare_exchange_weak(peak, current)) {
            // Loop until successful
        }
    }
    
    void track_deallocation(std::size_t size) {
        stats_.deallocations++;
        stats_.bytes_deallocated += size;
        stats_.current_usage -= size;
    }
    
    const Stats& get_stats() const { return stats_; }
    
    void reset() {
        stats_ = Stats{};
    }
    
    void print_report() const {
        std::cout << "Memory Usage Report:\n"
                  << "  Allocations: " << stats_.allocations << "\n"
                  << "  Deallocations: " << stats_.deallocations << "\n"
                  << "  Bytes allocated: " << stats_.bytes_allocated << "\n"
                  << "  Bytes deallocated: " << stats_.bytes_deallocated << "\n"
                  << "  Current usage: " << stats_.current_usage << "\n"
                  << "  Peak usage: " << stats_.peak_usage << "\n";
    }
    
private:
    MemoryTracker() = default;
    Stats stats_;
};

/**
 * @brief Tracked allocator wrapper
 * 
 * Wraps any allocator to track memory usage.
 */
template<typename Allocator>
class TrackedAllocator : public Allocator {
public:
    using typename Allocator::value_type;
    using typename Allocator::pointer;
    using typename Allocator::size_type;
    
    template<typename U>
    struct rebind {
        using other = TrackedAllocator<typename Allocator::template rebind<U>::other>;
    };
    
    TrackedAllocator() = default;
    TrackedAllocator(const TrackedAllocator&) = default;
    
    template<typename U>
    TrackedAllocator(const TrackedAllocator<U>&) noexcept {}
    
    pointer allocate(size_type n) {
        size_type bytes = n * sizeof(value_type);
        MemoryTracker::instance().track_allocation(bytes);
        return Allocator::allocate(n);
    }
    
    void deallocate(pointer p, size_type n) noexcept {
        size_type bytes = n * sizeof(value_type);
        MemoryTracker::instance().track_deallocation(bytes);
        Allocator::deallocate(p, n);
    }
};

} // namespace liarsdice::performance