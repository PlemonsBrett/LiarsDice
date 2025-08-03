#define BOOST_TEST_MODULE PerformanceTest
#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>

#include <liarsdice/performance/simd_operations.hpp>
#include <liarsdice/performance/custom_allocators.hpp>
#include <liarsdice/statistics/statistical_accumulator.hpp>
#include <liarsdice/data_structures/circular_buffer.hpp>
#include <liarsdice/data_structures/lru_cache.hpp>

#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

using namespace liarsdice::performance;
using namespace liarsdice::statistics;
using namespace liarsdice::data_structures;

// Helper to measure execution time and return nanoseconds
template<typename Func>
double measure_time(Func func, std::size_t iterations = 1) {
    boost::timer::cpu_timer timer;
    timer.start();
    
    for (std::size_t i = 0; i < iterations; ++i) {
        func();
    }
    
    timer.stop();
    return static_cast<double>(timer.elapsed().wall) / iterations;
}

// Helper to generate random data
template<typename T>
std::vector<T> generate_random_data(std::size_t size, T min = 0, T max = 100) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    if constexpr (std::is_floating_point_v<T>) {
        std::uniform_real_distribution<T> dist(min, max);
        std::vector<T> data(size);
        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
        return data;
    } else {
        std::uniform_int_distribution<T> dist(min, max);
        std::vector<T> data(size);
        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
        return data;
    }
}

BOOST_AUTO_TEST_SUITE(SimdPerformanceTests)

BOOST_AUTO_TEST_CASE(SimdVsScalarDotProduct) {
    const std::vector<std::size_t> sizes = {100, 1000, 10000, 100000};
    
    std::cout << "\n=== SIMD vs Scalar Dot Product Performance ===\n";
    std::cout << std::setw(10) << "Size" 
              << std::setw(20) << "Scalar (ns)"
              << std::setw(20) << "SIMD (ns)"
              << std::setw(15) << "Speedup"
              << "\n";
    
    for (auto size : sizes) {
        auto a = generate_random_data<float>(size);
        auto b = generate_random_data<float>(size);
        
        // Scalar version
        auto scalar_time = measure_time([&]() {
            float result = 0;
            for (std::size_t i = 0; i < size; ++i) {
                result += a[i] * b[i];
            }
            [[maybe_unused]] volatile float r = result;
        }, 1000);
        
        // SIMD version
        auto simd_time = measure_time([&]() {
            float result = SimdOperations<float>::dot_product(a, b);
            [[maybe_unused]] volatile float r = result;
        }, 1000);
        
        double speedup = scalar_time / simd_time;
        
        std::cout << std::setw(10) << size
                  << std::setw(20) << std::fixed << std::setprecision(2) << scalar_time
                  << std::setw(20) << std::fixed << std::setprecision(2) << simd_time
                  << std::setw(15) << std::fixed << std::setprecision(2) << speedup << "x"
                  << "\n";
                  
        // Performance validation: SIMD should be faster for large arrays
        if (size >= 1000) {
            BOOST_CHECK(simd_time < scalar_time);
        }
    }
}

BOOST_AUTO_TEST_CASE(SimdStatisticsPerformance) {
    const std::size_t size = 100000;
    auto data = generate_random_data<float>(size);
    
    std::cout << "\n=== SIMD Statistics Performance ===\n";
    
    // Standard calculation
    auto std_time = measure_time([&]() {
        double sum = 0;
        for (auto v : data) sum += v;
        double mean = sum / size;
        
        double var_sum = 0;
        for (auto v : data) {
            double diff = v - mean;
            var_sum += diff * diff;
        }
        [[maybe_unused]] volatile double variance = var_sum / (size - 1);
    }, 100);
    
    // SIMD calculation
    auto simd_time = measure_time([&]() {
        auto [mean, variance] = SimdOperations<float>::mean_variance(data);
        [[maybe_unused]] volatile double m = mean;
        [[maybe_unused]] volatile double v = variance;
    }, 100);
    
    double speedup = std_time / simd_time;
    
    std::cout << "Standard calculation: " << std::fixed << std::setprecision(2) 
              << std_time << " ns\n";
    std::cout << "SIMD calculation: " << std::fixed << std::setprecision(2) 
              << simd_time << " ns\n";
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
              
    BOOST_CHECK(simd_time < std_time);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(AllocatorPerformanceTests)

BOOST_AUTO_TEST_CASE(PoolAllocatorVsStandard) {
    const std::size_t iterations = 100000;
    
    std::cout << "\n=== Pool Allocator vs Standard Allocator ===\n";
    
    // Standard allocator
    auto std_time = measure_time([&]() {
        std::vector<std::unique_ptr<int>> ptrs;
        ptrs.reserve(100);
        
        for (int i = 0; i < 100; ++i) {
            ptrs.push_back(std::make_unique<int>(i));
        }
        ptrs.clear();
    }, iterations);
    
    // Pool allocator
    auto pool_time = measure_time([&]() {
        std::vector<int*> ptrs;
        FastPoolAllocator<int> alloc;
        ptrs.reserve(100);
        
        for (int i = 0; i < 100; ++i) {
            int* p = alloc.allocate(1);
            alloc.construct(p, i);
            ptrs.push_back(p);
        }
        
        for (auto p : ptrs) {
            alloc.destroy(p);
            alloc.deallocate(p, 1);
        }
    }, iterations);
    
    double speedup = std_time / pool_time;
    
    std::cout << "Standard allocator: " << std::fixed << std::setprecision(2) 
              << std_time << " ns\n";
    std::cout << "Pool allocator: " << std::fixed << std::setprecision(2) 
              << pool_time << " ns\n";
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
              
    BOOST_CHECK(pool_time < std_time);
}

BOOST_AUTO_TEST_CASE(MemoryArenaPerformance) {
    const std::size_t iterations = 10000;
    
    std::cout << "\n=== Memory Arena Performance ===\n";
    
    struct TestObject {
        int a, b, c;
        double x, y, z;
    };
    
    // Standard allocation
    auto std_time = measure_time([&]() {
        std::vector<std::unique_ptr<TestObject>> objects;
        for (int i = 0; i < 1000; ++i) {
            objects.push_back(std::make_unique<TestObject>());
        }
    }, iterations);
    
    // Arena allocation
    auto arena_time = measure_time([&]() {
        MemoryArena arena(1024 * 1024);
        std::vector<TestObject*> objects;
        
        for (int i = 0; i < 1000; ++i) {
            objects.push_back(arena.construct<TestObject>());
        }
        
        arena.reset(); // Bulk deallocation
    }, iterations);
    
    double speedup = std_time / arena_time;
    
    std::cout << "Standard allocation: " << std::fixed << std::setprecision(2) 
              << std_time << " ns\n";
    std::cout << "Arena allocation: " << std::fixed << std::setprecision(2) 
              << arena_time << " ns\n";
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
              
    BOOST_CHECK(arena_time < std_time * 0.5); // Arena should be significantly faster
}

BOOST_AUTO_TEST_CASE(SimdAllocatorAlignment) {
    using AlignedVector = std::vector<float, SimdAllocator<float, 32>>;
    
    AlignedVector vec(1000);
    
    // Check alignment
    auto ptr = vec.data();
    std::size_t address = reinterpret_cast<std::size_t>(ptr);
    
    BOOST_CHECK_EQUAL(address % 32, 0); // Should be 32-byte aligned
    
    // Performance test with aligned vs unaligned
    auto aligned_data = AlignedVector(10000);
    auto unaligned_data = std::vector<float>(10000);
    
    std::generate(aligned_data.begin(), aligned_data.end(), std::rand);
    std::copy(aligned_data.begin(), aligned_data.end(), unaligned_data.begin());
    
    auto aligned_time = measure_time([&]() {
        float sum = SimdOperations<float>::dot_product(aligned_data, aligned_data);
        [[maybe_unused]] volatile float s = sum;
    }, 1000);
    
    auto unaligned_time = measure_time([&]() {
        float sum = SimdOperations<float>::dot_product(unaligned_data, unaligned_data);
        [[maybe_unused]] volatile float s = sum;
    }, 1000);
    
    std::cout << "\n=== Aligned vs Unaligned Memory Access ===\n";
    std::cout << "Aligned access: " << std::fixed << std::setprecision(2) 
              << aligned_time << " ns\n";
    std::cout << "Unaligned access: " << std::fixed << std::setprecision(2) 
              << unaligned_time << " ns\n";
              
    // Aligned access should be at least as fast
    BOOST_CHECK(aligned_time <= unaligned_time * 1.1); // Allow 10% tolerance
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(DataStructurePerformanceTests)

BOOST_AUTO_TEST_CASE(LRUCachePerformance) {
    const std::size_t cache_size = 1000;
    const std::size_t operations = 100000;
    
    LRUCache<int, int> cache(cache_size);
    
    std::cout << "\n=== LRU Cache Performance ===\n";
    
    // Populate cache
    for (int i = 0; i < static_cast<int>(cache_size); ++i) {
        cache.put(i, i * 10);
    }
    
    // Random access pattern
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, cache_size * 2);
    
    auto access_time = measure_time([&]() {
        for (std::size_t i = 0; i < operations; ++i) {
            int key = dist(gen);
            [[maybe_unused]] auto val = cache.get(key);
        }
    });
    
    auto [hits, misses, evictions, hit_rate] = cache.get_stats();
    
    std::cout << "Operations: " << operations << "\n";
    std::cout << "Average access time: " << std::fixed << std::setprecision(2) 
              << access_time / operations << " ns\n";
    std::cout << "Hit rate: " << std::fixed << std::setprecision(2) 
              << hit_rate * 100 << "%\n";
    std::cout << "Evictions: " << evictions << "\n";
    
    // Should maintain O(1) access time
    double avg_time = access_time / operations;
    BOOST_CHECK(avg_time < 1000); // Less than 1 microsecond per operation
}

BOOST_AUTO_TEST_CASE(CircularBufferPerformance) {
    const std::size_t buffer_size = 10000;
    const std::size_t iterations = 1000000;
    
    CircularBuffer<double> buffer(buffer_size);
    
    std::cout << "\n=== Circular Buffer Performance ===\n";
    
    // Push performance
    auto push_time = measure_time([&]() {
        for (std::size_t i = 0; i < iterations; ++i) {
            buffer.push_back(static_cast<double>(i));
        }
    });
    
    // Window analysis performance
    buffer.clear();
    for (std::size_t i = 0; i < buffer_size; ++i) {
        buffer.push_back(static_cast<double>(i));
    }
    
    auto window_time = measure_time([&]() {
        auto window = buffer.get_window(100);
        [[maybe_unused]] volatile std::size_t size = window.size();
    }, 10000);
    
    // Statistics calculation performance
    auto stats_time = measure_time([&]() {
        auto [mean, std_dev, min, max] = buffer.calculate_statistics(
            [](double x) { return x; });
        [[maybe_unused]] volatile double m = mean;
    }, 1000);
    
    std::cout << "Push time per element: " << std::fixed << std::setprecision(2) 
              << push_time / iterations << " ns\n";
    std::cout << "Window extraction (100 elements): " << std::fixed << std::setprecision(2) 
              << window_time << " ns\n";
    std::cout << "Statistics calculation: " << std::fixed << std::setprecision(2) 
              << stats_time << " ns\n";
    
    // Performance requirements
    BOOST_CHECK(push_time / iterations < 100); // Less than 100ns per push
    BOOST_CHECK(window_time < 10000); // Less than 10us for window
}

BOOST_AUTO_TEST_CASE(StatisticalAccumulatorPerformance) {
    const std::size_t data_size = 1000000;
    
    StatisticalAccumulator<double> acc;
    auto data = generate_random_data<double>(data_size);
    
    std::cout << "\n=== Statistical Accumulator Performance ===\n";
    
    // Single-pass statistics
    auto acc_time = measure_time([&]() {
        StatisticalAccumulator<double> temp_acc;
        for (auto v : data) {
            temp_acc.add(v);
        }
        [[maybe_unused]] auto stats = temp_acc.get_statistics();
    });
    
    // Traditional two-pass calculation
    auto trad_time = measure_time([&]() {
        // First pass: mean
        double sum = 0;
        for (auto v : data) sum += v;
        double mean = sum / data_size;
        
        // Second pass: variance
        double var_sum = 0;
        for (auto v : data) {
            double diff = v - mean;
            var_sum += diff * diff;
        }
        [[maybe_unused]] volatile double variance = var_sum / (data_size - 1);
        
        // Min/max
        auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
        [[maybe_unused]] volatile double min_val = *min_it;
        [[maybe_unused]] volatile double max_val = *max_it;
    });
    
    double speedup = trad_time / acc_time;
    
    std::cout << "Accumulator (single-pass): " << std::fixed << std::setprecision(2) 
              << acc_time << " ns\n";
    std::cout << "Traditional (multi-pass): " << std::fixed << std::setprecision(2) 
              << trad_time << " ns\n";
    std::cout << "Efficiency gain: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
    
    // Single-pass should be more efficient
    BOOST_CHECK(acc_time < trad_time);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(MemoryProfilingTests)

BOOST_AUTO_TEST_CASE(MemoryUsageTracking) {
    std::cout << "\n=== Memory Usage Profiling ===\n";
    
    // Reset tracker
    MemoryTracker::instance().reset();
    
    // Test with tracked allocator
    using TrackedVector = std::vector<int, TrackedAllocator<FastPoolAllocator<int>>>;
    
    {
        TrackedVector vec;
        for (int i = 0; i < 10000; ++i) {
            vec.push_back(i);
        }
        
        // Force reallocation
        vec.shrink_to_fit();
    }
    
    auto& stats = MemoryTracker::instance().get_stats();
    
    std::cout << "Allocations: " << stats.allocations << "\n";
    std::cout << "Deallocations: " << stats.deallocations << "\n";
    std::cout << "Total allocated: " << stats.bytes_allocated << " bytes\n";
    std::cout << "Total deallocated: " << stats.bytes_deallocated << " bytes\n";
    std::cout << "Peak usage: " << stats.peak_usage << " bytes\n";
    std::cout << "Current usage: " << stats.current_usage << " bytes\n";
    
    // Verify tracking
    BOOST_CHECK_GT(stats.allocations.load(), 0);
    BOOST_CHECK_GT(stats.bytes_allocated.load(), 0);
    
    // Memory should be freed
    BOOST_CHECK_EQUAL(stats.allocations.load(), stats.deallocations.load());
}

BOOST_AUTO_TEST_CASE(PoolAllocatorStatistics) {
    std::cout << "\n=== Pool Allocator Statistics ===\n";
    
    // Reset pool
    FastPoolAllocator<int>::release_memory();
    
    {
        std::vector<int*> vec;
        FastPoolAllocator<int> alloc;
        
        // Allocate many objects
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(alloc.allocate(1));
        }
        
        std::cout << "Allocated objects: " << FastPoolAllocator<int>::get_allocated_count() << "\n";
        
        // Deallocate half
        for (int i = 0; i < 500; ++i) {
            alloc.deallocate(vec[i], 1);
        }
        
        // Allocate more
        for (int i = 0; i < 500; ++i) {
            vec[i] = alloc.allocate(1);
        }
        
        // Clean up
        for (auto p : vec) {
            alloc.deallocate(p, 1);
        }
    }
    
    // Memory should be pooled for reuse
    auto allocated_after = FastPoolAllocator<int>::get_allocated_count();
    std::cout << "Pool size after cleanup: " << allocated_after << "\n";
    
    // Purge memory
    FastPoolAllocator<int>::purge_memory();
    
    std::cout << "Pool size after purge: " 
              << FastPoolAllocator<int>::get_allocated_count() << "\n";
}

BOOST_AUTO_TEST_SUITE_END()