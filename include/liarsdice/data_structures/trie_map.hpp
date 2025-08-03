#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace liarsdice::data_structures {

/**
 * @brief High-performance Trie data structure for pattern storage
 * 
 * Uses Boost flat_map for cache-efficient child node storage.
 * Optimized for storing and retrieving player behavior patterns.
 * 
 * @tparam T Value type to store at pattern endpoints
 */
template<typename T>
class TrieMap {
private:
    struct TrieNode {
        boost::optional<T> value;
        boost::container::flat_map<char, std::unique_ptr<TrieNode>> children;
        
        TrieNode() = default;
    };
    
public:
    TrieMap() : root_(std::make_unique<TrieNode>()) {}
    
    /**
     * @brief Insert a pattern-value pair into the trie
     * @param pattern The pattern string to insert
     * @param value The value to associate with the pattern
     */
    void insert(const std::string& pattern, T value) {
        TrieNode* current = root_.get();
        
        for (char ch : pattern) {
            auto& child = current->children[ch];
            if (!child) {
                child = std::make_unique<TrieNode>();
            }
            current = child.get();
        }
        
        current->value = std::move(value);
    }
    
    /**
     * @brief Find a value by exact pattern match
     * @param pattern The pattern to search for
     * @return Optional containing the value if found
     */
    [[nodiscard]] boost::optional<T> find(const std::string& pattern) const {
        const TrieNode* current = root_.get();
        
        for (char ch : pattern) {
            auto it = current->children.find(ch);
            if (it == current->children.end()) {
                return boost::none;
            }
            current = it->second.get();
        }
        
        return current->value;
    }
    
    /**
     * @brief Find all values with patterns that are prefixes of the input
     * @param text The text to search within
     * @return Vector of found values
     */
    [[nodiscard]] std::vector<T> find_prefixes(const std::string& text) const {
        std::vector<T> results;
        const TrieNode* current = root_.get();
        
        for (char ch : text) {
            auto it = current->children.find(ch);
            if (it == current->children.end()) {
                break;
            }
            current = it->second.get();
            
            if (current->value) {
                results.push_back(*current->value);
            }
        }
        
        return results;
    }
    
    /**
     * @brief Remove a pattern from the trie
     * @param pattern The pattern to remove
     * @return True if the pattern was found and removed
     */
    bool erase(const std::string& pattern) {
        return erase_helper(root_.get(), pattern, 0);
    }
    
    /**
     * @brief Clear all patterns from the trie
     */
    void clear() {
        root_ = std::make_unique<TrieNode>();
    }
    
    /**
     * @brief Check if the trie is empty
     * @return True if no patterns are stored
     */
    [[nodiscard]] bool empty() const {
        return root_->children.empty() && !root_->value;
    }
    
    /**
     * @brief Get all stored patterns and their values
     * @return Vector of pattern-value pairs
     */
    [[nodiscard]] std::vector<std::pair<std::string, T>> get_all() const {
        std::vector<std::pair<std::string, T>> results;
        std::string current_pattern;
        collect_all(root_.get(), current_pattern, results);
        return results;
    }
    
private:
    std::unique_ptr<TrieNode> root_;
    
    bool erase_helper(TrieNode* node, const std::string& pattern, size_t index) {
        if (index == pattern.size()) {
            if (!node->value) {
                return false;
            }
            node->value = boost::none;
            return node->children.empty();
        }
        
        auto it = node->children.find(pattern[index]);
        if (it == node->children.end()) {
            return false;
        }
        
        bool should_delete_child = erase_helper(it->second.get(), pattern, index + 1);
        
        if (should_delete_child) {
            node->children.erase(it);
        }
        
        return node->children.empty() && !node->value;
    }
    
    void collect_all(const TrieNode* node, std::string& current, 
                     std::vector<std::pair<std::string, T>>& results) const {
        if (node->value) {
            results.emplace_back(current, *node->value);
        }
        
        for (const auto& [ch, child] : node->children) {
            current.push_back(ch);
            collect_all(child.get(), current, results);
            current.pop_back();
        }
    }
};

/**
 * @brief Specialized TrieMap for player behavior patterns
 * 
 * Stores pattern strings mapped to behavior statistics
 */
struct BehaviorPattern {
    double frequency = 0.0;
    double success_rate = 0.0;
    size_t occurrences = 0;
};

using PlayerPatternTrie = TrieMap<BehaviorPattern>;

} // namespace liarsdice::data_structures