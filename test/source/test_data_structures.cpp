#define BOOST_TEST_MODULE DataStructuresTest
#include <boost/test/unit_test.hpp>
#include <liarsdice/data_structures/circular_buffer.hpp>
#include <liarsdice/data_structures/lru_cache.hpp>
#include <liarsdice/data_structures/sparse_matrix.hpp>
#include <liarsdice/data_structures/trie_map.hpp>

using namespace liarsdice::data_structures;

BOOST_AUTO_TEST_SUITE(TrieMapTests)

BOOST_AUTO_TEST_CASE(BasicOperations) {
  TrieMap<int> trie;

  // Test insertion and lookup
  trie.insert("hello", 42);
  trie.insert("help", 17);
  trie.insert("world", 99);

  BOOST_CHECK_EQUAL(*trie.find("hello"), 42);
  BOOST_CHECK_EQUAL(*trie.find("help"), 17);
  BOOST_CHECK_EQUAL(*trie.find("world"), 99);
  BOOST_CHECK(!trie.find("hell"));
}

BOOST_AUTO_TEST_CASE(PrefixMatching) {
  TrieMap<std::string> trie;

  trie.insert("a", "first");
  trie.insert("ab", "second");
  trie.insert("abc", "third");

  auto prefixes = trie.find_prefixes("abcd");
  BOOST_CHECK_EQUAL(prefixes.size(), 3);
  BOOST_CHECK_EQUAL(prefixes[0], "first");
  BOOST_CHECK_EQUAL(prefixes[1], "second");
  BOOST_CHECK_EQUAL(prefixes[2], "third");
}

BOOST_AUTO_TEST_CASE(PlayerPatternStorage) {
  PlayerPatternTrie patterns;

  BehaviorPattern pattern1{0.75, 0.80, 10};
  BehaviorPattern pattern2{0.60, 0.70, 15};

  patterns.insert("GGC", pattern1);
  patterns.insert("CLL", pattern2);

  auto found = patterns.find("GGC");
  BOOST_REQUIRE(found);
  BOOST_CHECK_CLOSE(found->frequency, 0.75, 0.001);
  BOOST_CHECK_CLOSE(found->success_rate, 0.80, 0.001);
  BOOST_CHECK_EQUAL(found->occurrences, 10);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(CircularBufferTests)

BOOST_AUTO_TEST_CASE(BasicOperations) {
  CircularBuffer<int> buffer(5);

  // Test push operations
  for (int i = 1; i <= 7; ++i) {
    buffer.push_back(i);
  }

  // Should contain [3, 4, 5, 6, 7]
  BOOST_CHECK_EQUAL(buffer.size(), 5);
  BOOST_CHECK_EQUAL(buffer.front(), 3);
  BOOST_CHECK_EQUAL(buffer.back(), 7);
}

BOOST_AUTO_TEST_CASE(PerfectForwarding) {
  struct TestStruct {
    int a;
    double b;
    TestStruct(int x, double y) : a(x), b(y) {}
  };

  CircularBuffer<TestStruct> buffer(3);

  buffer.emplace_back(1, 2.5);
  buffer.emplace_back(3, 4.5);

  BOOST_CHECK_EQUAL(buffer.front().a, 1);
  BOOST_CHECK_CLOSE(buffer.front().b, 2.5, 0.001);
}

BOOST_AUTO_TEST_CASE(WindowAnalysis) {
  CircularBuffer<int> buffer(10);

  for (int i = 1; i <= 10; ++i) {
    buffer.push_back(i);
  }

  auto window = buffer.get_window(3);
  BOOST_CHECK_EQUAL(window.size(), 3);
  BOOST_CHECK_EQUAL(window[0], 8);
  BOOST_CHECK_EQUAL(window[1], 9);
  BOOST_CHECK_EQUAL(window[2], 10);
}

BOOST_AUTO_TEST_CASE(Statistics) {
  CircularBuffer<double> buffer(5);

  buffer.push_back(1.0);
  buffer.push_back(2.0);
  buffer.push_back(3.0);
  buffer.push_back(4.0);
  buffer.push_back(5.0);

  auto [mean, std_dev, min_val, max_val] = buffer.calculate_statistics([](double x) { return x; });

  BOOST_CHECK_CLOSE(mean, 3.0, 0.001);
  BOOST_CHECK_CLOSE(min_val, 1.0, 0.001);
  BOOST_CHECK_CLOSE(max_val, 5.0, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(SparseMatrixTests)

BOOST_AUTO_TEST_CASE(BasicOperations) {
  SparseMatrix<double> matrix(3, 3);

  matrix.set(0, 0, 1.0);
  matrix.set(1, 1, 2.0);
  matrix.set(2, 2, 3.0);

  BOOST_CHECK_CLOSE(matrix.get(0, 0), 1.0, 0.001);
  BOOST_CHECK_CLOSE(matrix.get(1, 1), 2.0, 0.001);
  BOOST_CHECK_CLOSE(matrix.get(2, 2), 3.0, 0.001);
  BOOST_CHECK_EQUAL(matrix.get(0, 1), 0.0);

  BOOST_CHECK_EQUAL(matrix.non_zeros(), 3);
  BOOST_CHECK_CLOSE(matrix.sparsity(), 2.0 / 3.0, 0.001);
}

BOOST_AUTO_TEST_CASE(RowColumnOperations) {
  SparseMatrix<int> matrix(3, 3);

  matrix.set(0, 0, 1);
  matrix.set(0, 1, 2);
  matrix.set(1, 1, 3);
  matrix.set(2, 0, 4);

  auto row_sums = matrix.row_sums();
  BOOST_CHECK_EQUAL(row_sums[0], 3);  // 1 + 2
  BOOST_CHECK_EQUAL(row_sums[1], 3);  // 3
  BOOST_CHECK_EQUAL(row_sums[2], 4);  // 4

  auto col_sums = matrix.column_sums();
  BOOST_CHECK_EQUAL(col_sums[0], 5);  // 1 + 4
  BOOST_CHECK_EQUAL(col_sums[1], 5);  // 2 + 3
  BOOST_CHECK_EQUAL(col_sums[2], 0);  // empty column
}

BOOST_AUTO_TEST_CASE(TopNElements) {
  SparseMatrix<double> matrix(4, 4);

  matrix.set(0, 0, 5.0);
  matrix.set(1, 1, 3.0);
  matrix.set(2, 2, 8.0);
  matrix.set(3, 3, 1.0);

  auto top3 = matrix.find_top_n(3);
  BOOST_CHECK_EQUAL(top3.size(), 3);
  BOOST_CHECK_CLOSE(std::get<2>(top3[0]), 8.0, 0.001);
  BOOST_CHECK_CLOSE(std::get<2>(top3[1]), 5.0, 0.001);
  BOOST_CHECK_CLOSE(std::get<2>(top3[2]), 3.0, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(LRUCacheTests)

BOOST_AUTO_TEST_CASE(BasicOperations) {
  LRUCache<int, std::string> cache(3);

  cache.put(1, "one");
  cache.put(2, "two");
  cache.put(3, "three");

  BOOST_CHECK_EQUAL(*cache.get(1), "one");
  BOOST_CHECK_EQUAL(*cache.get(2), "two");
  BOOST_CHECK_EQUAL(*cache.get(3), "three");

  // Access 1 to make it most recently used
  [[maybe_unused]] auto accessed = cache.get(1);

  // Add fourth element, should evict 2
  cache.put(4, "four");

  BOOST_CHECK(cache.get(1));
  BOOST_CHECK(!cache.get(2));  // Evicted
  BOOST_CHECK(cache.get(3));
  BOOST_CHECK(cache.get(4));
}

BOOST_AUTO_TEST_CASE(CacheStatistics) {
  LRUCache<int, int> cache(2);

  cache.put(1, 10);
  cache.put(2, 20);

  // Hits
  [[maybe_unused]] auto hit1 = cache.get(1);
  [[maybe_unused]] auto hit2 = cache.get(2);

  // Misses
  [[maybe_unused]] auto miss1 = cache.get(3);
  [[maybe_unused]] auto miss2 = cache.get(4);

  // Eviction
  cache.put(3, 30);  // Evicts 1

  auto [hits, misses, evictions, hit_rate] = cache.get_stats();
  BOOST_CHECK_EQUAL(hits, 2);
  BOOST_CHECK_EQUAL(misses, 2);
  BOOST_CHECK_EQUAL(evictions, 1);
  BOOST_CHECK_CLOSE(hit_rate, 0.5, 0.001);
}

BOOST_AUTO_TEST_CASE(ResizeCache) {
  LRUCache<int, int> cache(5);

  for (int i = 1; i <= 5; ++i) {
    cache.put(i, i * 10);
  }

  BOOST_CHECK_EQUAL(cache.size(), 5);

  // Resize to smaller capacity
  cache.resize(3);

  BOOST_CHECK_EQUAL(cache.size(), 3);
  BOOST_CHECK(!cache.get(1));  // Evicted
  BOOST_CHECK(!cache.get(2));  // Evicted
  BOOST_CHECK(cache.get(3));
  BOOST_CHECK(cache.get(4));
  BOOST_CHECK(cache.get(5));
}

BOOST_AUTO_TEST_SUITE_END()