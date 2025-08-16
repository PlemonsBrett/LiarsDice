#ifndef LIARSDICE_PREPARED_STATEMENT_HPP
#define LIARSDICE_PREPARED_STATEMENT_HPP

#include <sqlite3.h>

#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace liarsdice::database {

  /**
   * @brief RAII wrapper for SQLite prepared statements
   */
  class PreparedStatement : private boost::noncopyable {
  public:
    using ValueType
        = std::variant<std::monostate, int, int64_t, double, std::string, std::vector<uint8_t>>;

    /**
     * @brief Constructor
     * @param stmt SQLite statement (takes ownership)
     * @param sql Original SQL for reference
     */
    PreparedStatement(sqlite3_stmt* stmt, const std::string& sql)
        : stmt_(stmt, &sqlite3_finalize), sql_(sql) {}

    /**
     * @brief Bind parameter by index (1-based)
     */
    bool bind(int index, const ValueType& value) {
      if (!stmt_) return false;

      return std::visit(
          [this, index](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
              return sqlite3_bind_null(stmt_.get(), index) == SQLITE_OK;
            } else if constexpr (std::is_same_v<T, int>) {
              return sqlite3_bind_int(stmt_.get(), index, arg) == SQLITE_OK;
            } else if constexpr (std::is_same_v<T, int64_t>) {
              return sqlite3_bind_int64(stmt_.get(), index, arg) == SQLITE_OK;
            } else if constexpr (std::is_same_v<T, double>) {
              return sqlite3_bind_double(stmt_.get(), index, arg) == SQLITE_OK;
            } else if constexpr (std::is_same_v<T, std::string>) {
              return sqlite3_bind_text(stmt_.get(), index, arg.c_str(),
                                       static_cast<int>(arg.size()), SQLITE_TRANSIENT)
                     == SQLITE_OK;
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
              return sqlite3_bind_blob(stmt_.get(), index, arg.data(), static_cast<int>(arg.size()),
                                       SQLITE_TRANSIENT)
                     == SQLITE_OK;
            }
            return false;
          },
          value);
    }

    /**
     * @brief Bind parameter by name
     */
    bool bind(const std::string& name, const ValueType& value) {
      int index = sqlite3_bind_parameter_index(stmt_.get(), name.c_str());
      if (index == 0) return false;
      return bind(index, value);
    }

    /**
     * @brief Reset statement for reuse
     */
    bool reset() { return stmt_ && sqlite3_reset(stmt_.get()) == SQLITE_OK; }

    /**
     * @brief Clear all bindings
     */
    bool clear_bindings() { return stmt_ && sqlite3_clear_bindings(stmt_.get()) == SQLITE_OK; }

    /**
     * @brief Execute a step
     * @return SQLITE_ROW, SQLITE_DONE, or error code
     */
    int step() { return stmt_ ? sqlite3_step(stmt_.get()) : SQLITE_MISUSE; }

    /**
     * @brief Get column count
     */
    int column_count() const { return stmt_ ? sqlite3_column_count(stmt_.get()) : 0; }

    /**
     * @brief Get column value by index
     */
    ValueType get_column(int index) const {
      if (!stmt_ || index < 0 || index >= column_count()) {
        return std::monostate{};
      }

      int type = sqlite3_column_type(stmt_.get(), index);
      switch (type) {
        case SQLITE_NULL:
          return std::monostate{};
        case SQLITE_INTEGER:
          return sqlite3_column_int64(stmt_.get(), index);
        case SQLITE_FLOAT:
          return sqlite3_column_double(stmt_.get(), index);
        case SQLITE_TEXT: {
          const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_.get(), index));
          return text ? std::string(text) : std::string{};
        }
        case SQLITE_BLOB: {
          const void* blob = sqlite3_column_blob(stmt_.get(), index);
          int size = sqlite3_column_bytes(stmt_.get(), index);
          if (blob && size > 0) {
            auto data = static_cast<const uint8_t*>(blob);
            return std::vector<uint8_t>(data, data + size);
          }
          return std::vector<uint8_t>{};
        }
        default:
          return std::monostate{};
      }
    }

    /**
     * @brief Get column name
     */
    std::string get_column_name(int index) const {
      if (!stmt_ || index < 0 || index >= column_count()) {
        return "";
      }
      const char* name = sqlite3_column_name(stmt_.get(), index);
      return name ? name : "";
    }

    /**
     * @brief Get the original SQL
     */
    const std::string& sql() const { return sql_; }

    /**
     * @brief Get the underlying statement handle
     */
    sqlite3_stmt* get() const { return stmt_.get(); }

    /**
     * @brief Check if statement is valid
     */
    explicit operator bool() const { return stmt_ != nullptr; }

  private:
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt_;
    std::string sql_;
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_PREPARED_STATEMENT_HPP