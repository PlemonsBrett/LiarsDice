#pragma once

#include <algorithm>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>

namespace liarsdice::data_structures {

  namespace ublas = boost::numeric::ublas;

  /**
   * @brief High-performance sparse matrix for game analytics
   *
   * Uses boost::numeric::ublas::compressed_matrix for efficient storage
   * of sparse data like player interaction patterns and game statistics.
   *
   * @tparam T Element type (default: double)
   */
  template <typename T = double> class SparseMatrix {
  public:
    using value_type = T;
    using matrix_type = ublas::compressed_matrix<T>;
    using size_type = typename matrix_type::size_type;

    /**
     * @brief Construct sparse matrix with dimensions
     * @param rows Number of rows
     * @param cols Number of columns
     * @param non_zeros Expected number of non-zero elements (hint)
     */
    SparseMatrix(size_type rows, size_type cols, size_type non_zeros = 0)
        : matrix_(rows, cols, non_zeros) {}

    /**
     * @brief Set element value
     * @param row Row index
     * @param col Column index
     * @param value Value to set
     */
    void set(size_type row, size_type col, const T& value) {
      if (value != T{}) {  // Only store non-zero values
        matrix_(row, col) = value;
      } else {
        // Remove zero values to maintain sparsity
        matrix_.erase_element(row, col);
      }
    }

    /**
     * @brief Get element value
     * @param row Row index
     * @param col Column index
     * @return Element value (zero if not stored)
     */
    [[nodiscard]] T get(size_type row, size_type col) const { return matrix_(row, col); }

    /**
     * @brief Increment element value
     * @param row Row index
     * @param col Column index
     * @param delta Value to add
     */
    void increment(size_type row, size_type col, const T& delta = T{1}) {
      matrix_(row, col) += delta;
    }

    /**
     * @brief Get row as dense vector
     * @param row Row index
     * @return Vector containing row values
     */
    [[nodiscard]] std::vector<T> get_row(size_type row) const {
      std::vector<T> result(matrix_.size2(), T{});
      for (size_type col = 0; col < matrix_.size2(); ++col) {
        result[col] = matrix_(row, col);
      }
      return result;
    }

    /**
     * @brief Get column as dense vector
     * @param col Column index
     * @return Vector containing column values
     */
    [[nodiscard]] std::vector<T> get_column(size_type col) const {
      std::vector<T> result(matrix_.size1(), T{});
      for (size_type row = 0; row < matrix_.size1(); ++row) {
        result[row] = matrix_(row, col);
      }
      return result;
    }

    /**
     * @brief Calculate row sums
     * @return Vector of row sums
     */
    [[nodiscard]] std::vector<T> row_sums() const {
      std::vector<T> sums(matrix_.size1(), T{});

      // Iterate over non-zero elements efficiently
      for (auto it1 = matrix_.begin1(); it1 != matrix_.end1(); ++it1) {
        for (auto it2 = it1.begin(); it2 != it1.end(); ++it2) {
          sums[it2.index1()] += *it2;
        }
      }

      return sums;
    }

    /**
     * @brief Calculate column sums
     * @return Vector of column sums
     */
    [[nodiscard]] std::vector<T> column_sums() const {
      std::vector<T> sums(matrix_.size2(), T{});

      for (auto it1 = matrix_.begin1(); it1 != matrix_.end1(); ++it1) {
        for (auto it2 = it1.begin(); it2 != it1.end(); ++it2) {
          sums[it2.index2()] += *it2;
        }
      }

      return sums;
    }

    /**
     * @brief Apply function to all non-zero elements
     * @tparam Func Function type
     * @param func Function to apply (row, col, value)
     */
    template <typename Func> void for_each_non_zero(Func func) const {
      for (auto it1 = matrix_.begin1(); it1 != matrix_.end1(); ++it1) {
        for (auto it2 = it1.begin(); it2 != it1.end(); ++it2) {
          func(it2.index1(), it2.index2(), *it2);
        }
      }
    }

    /**
     * @brief Find top N elements by value
     * @param n Number of elements to find
     * @return Vector of (row, col, value) tuples
     */
    [[nodiscard]] std::vector<std::tuple<size_type, size_type, T>> find_top_n(size_type n) const {
      std::vector<std::tuple<size_type, size_type, T>> elements;

      for_each_non_zero([&elements](size_type row, size_type col, const T& val) {
        elements.emplace_back(row, col, val);
      });

      std::partial_sort(
          elements.begin(), elements.begin() + std::min(n, elements.size()), elements.end(),
          [](const auto& a, const auto& b) { return std::get<2>(a) > std::get<2>(b); });

      if (elements.size() > n) {
        elements.resize(n);
      }

      return elements;
    }

    /**
     * @brief Normalize matrix rows to sum to 1
     * @return New normalized matrix
     */
    [[nodiscard]] SparseMatrix normalize_rows() const {
      SparseMatrix result(rows(), cols(), non_zeros());
      auto sums = row_sums();

      for_each_non_zero([&result, &sums](size_type row, size_type col, const T& val) {
        if (sums[row] != T{}) {
          result.set(row, col, val / sums[row]);
        }
      });

      return result;
    }

    /**
     * @brief Matrix multiplication
     * @param other Other sparse matrix
     * @return Result of matrix multiplication
     */
    [[nodiscard]] SparseMatrix operator*(const SparseMatrix& other) const {
      if (cols() != other.rows()) {
        throw std::invalid_argument("Matrix dimensions incompatible for multiplication");
      }

      SparseMatrix result(rows(), other.cols());
      result.matrix_ = ublas::prod(matrix_, other.matrix_);
      return result;
    }

    // Matrix properties
    [[nodiscard]] size_type rows() const { return matrix_.size1(); }
    [[nodiscard]] size_type cols() const { return matrix_.size2(); }
    [[nodiscard]] size_type non_zeros() const { return matrix_.nnz(); }
    [[nodiscard]] double sparsity() const {
      return 1.0 - static_cast<double>(non_zeros()) / (rows() * cols());
    }

    // Clear matrix
    void clear() { matrix_.clear(); }

    // Access underlying matrix
    [[nodiscard]] const matrix_type& data() const { return matrix_; }
    [[nodiscard]] matrix_type& data() { return matrix_; }

  private:
    matrix_type matrix_;
  };

  /**
   * @brief Specialized sparse matrix for player interaction tracking
   *
   * Tracks interactions between players (calls, bluffs, etc.)
   */
  using PlayerInteractionMatrix = SparseMatrix<unsigned int>;

  /**
   * @brief Specialized sparse matrix for probability calculations
   *
   * Used for transition matrices and statistical models
   */
  using ProbabilityMatrix = SparseMatrix<double>;

}  // namespace liarsdice::data_structures