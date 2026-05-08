/**
 *
 *  @file KeyPath.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Structured hierarchical key path
 *
 */

#ifndef VIX_KV_KEYS_KEY_PATH_HPP
#define VIX_KV_KEYS_KEY_PATH_HPP

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::kv::keys
{
  /**
   * @brief Structured hierarchical key path for Vix KV.
   *
   * KeyPath is the public developer-facing key representation.
   *
   * Example:
   * @code
   * KeyPath key{"users", "42", "profile"};
   * @endcode
   *
   * It is later validated and encoded into a stable internal key format.
   *
   * Rules:
   * - A valid key path should contain at least one segment.
   * - Empty segments are not valid for storage.
   * - Validation is handled by KeyValidator.
   * - Encoding is handled by KeyEncoder.
   */
  class KeyPath
  {
  public:
    /**
     * @brief One key path segment.
     */
    using Segment = std::string;

    /**
     * @brief Internal segment container.
     */
    using Container = std::vector<Segment>;

    /**
     * @brief Creates an empty key path.
     */
    KeyPath() = default;

    /**
     * @brief Creates a key path from string view segments.
     *
     * Example:
     * @code
     * KeyPath key{"users", "42", "profile"};
     * @endcode
     *
     * @param parts Ordered key segments.
     */
    KeyPath(std::initializer_list<std::string_view> parts)
    {
      segments_.reserve(parts.size());

      for (std::string_view part : parts)
      {
        segments_.emplace_back(part);
      }
    }

    /**
     * @brief Creates a key path from string segments.
     *
     * @param parts Ordered key segments.
     */
    KeyPath(std::initializer_list<std::string> parts)
    {
      segments_.reserve(parts.size());

      for (const std::string &part : parts)
      {
        segments_.push_back(part);
      }
    }

    /**
     * @brief Creates a key path from an existing container.
     *
     * @param parts Ordered key segments.
     */
    explicit KeyPath(Container parts)
        : segments_(std::move(parts))
    {
    }

    /**
     * @brief Creates a key path with a single segment.
     *
     * @param segment Single key segment.
     * @return KeyPath instance.
     */
    [[nodiscard]] static KeyPath from(std::string_view segment)
    {
      KeyPath path;
      path.push_back(segment);
      return path;
    }

    /**
     * @brief Creates a key path from a vector of segments.
     *
     * @param parts Ordered key segments.
     * @return KeyPath instance.
     */
    [[nodiscard]] static KeyPath from_segments(Container parts)
    {
      return KeyPath(std::move(parts));
    }

    /**
     * @brief Appends one segment.
     *
     * @param segment Segment to append.
     * @return Reference to this key path.
     */
    KeyPath &push_back(std::string_view segment)
    {
      segments_.emplace_back(segment);
      return *this;
    }

    /**
     * @brief Appends one segment by move.
     *
     * @param segment Segment to append.
     * @return Reference to this key path.
     */
    KeyPath &push_back(std::string segment)
    {
      segments_.push_back(std::move(segment));
      return *this;
    }

    /**
     * @brief Appends one segment.
     *
     * Alias for push_back().
     *
     * @param segment Segment to append.
     * @return Reference to this key path.
     */
    KeyPath &append(std::string_view segment)
    {
      return push_back(segment);
    }

    /**
     * @brief Appends all segments from another key path.
     *
     * @param other Key path to append.
     * @return Reference to this key path.
     */
    KeyPath &append(const KeyPath &other)
    {
      segments_.reserve(segments_.size() + other.segments_.size());

      for (const auto &segment : other.segments_)
      {
        segments_.push_back(segment);
      }

      return *this;
    }

    /**
     * @brief Removes the last segment.
     *
     * Does nothing when the path is empty.
     */
    void pop_back()
    {
      if (!segments_.empty())
      {
        segments_.pop_back();
      }
    }

    /**
     * @brief Returns the number of segments.
     *
     * @return Segment count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return segments_.size();
    }

    /**
     * @brief Returns true when the key path has no segment.
     *
     * @return true when empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return segments_.empty();
    }

    /**
     * @brief Returns the total number of segment bytes.
     *
     * This does not include encoded separators or length prefixes.
     *
     * @return Total segment bytes.
     */
    [[nodiscard]] std::size_t byte_size() const noexcept
    {
      std::size_t total = 0;

      for (const auto &segment : segments_)
      {
        total += segment.size();
      }

      return total;
    }

    /**
     * @brief Returns a segment by index.
     *
     * @param index Segment index.
     * @return Constant segment reference.
     */
    [[nodiscard]] const Segment &at(std::size_t index) const
    {
      return segments_.at(index);
    }

    /**
     * @brief Returns a mutable segment by index.
     *
     * @param index Segment index.
     * @return Mutable segment reference.
     */
    [[nodiscard]] Segment &at(std::size_t index)
    {
      return segments_.at(index);
    }

    /**
     * @brief Returns the first segment.
     *
     * The caller must ensure the path is not empty.
     *
     * @return Constant first segment reference.
     */
    [[nodiscard]] const Segment &front() const
    {
      return segments_.front();
    }

    /**
     * @brief Returns the first segment.
     *
     * The caller must ensure the path is not empty.
     *
     * @return Mutable first segment reference.
     */
    [[nodiscard]] Segment &front()
    {
      return segments_.front();
    }

    /**
     * @brief Returns the last segment.
     *
     * The caller must ensure the path is not empty.
     *
     * @return Constant last segment reference.
     */
    [[nodiscard]] const Segment &back() const
    {
      return segments_.back();
    }

    /**
     * @brief Returns the last segment.
     *
     * The caller must ensure the path is not empty.
     *
     * @return Mutable last segment reference.
     */
    [[nodiscard]] Segment &back()
    {
      return segments_.back();
    }

    /**
     * @brief Returns all segments.
     *
     * @return Read-only segment container.
     */
    [[nodiscard]] const Container &segments() const noexcept
    {
      return segments_;
    }

    /**
     * @brief Returns mutable access to all segments.
     *
     * @return Mutable segment container.
     */
    [[nodiscard]] Container &segments() noexcept
    {
      return segments_;
    }

    /**
     * @brief Reserves capacity for segments.
     *
     * @param capacity Segment capacity.
     */
    void reserve(std::size_t capacity)
    {
      segments_.reserve(capacity);
    }

    /**
     * @brief Removes all segments.
     */
    void clear() noexcept
    {
      segments_.clear();
    }

    /**
     * @brief Compares two key paths for equality.
     *
     * @param other Other key path.
     * @return true if both paths have the same segments.
     */
    [[nodiscard]] bool operator==(const KeyPath &other) const noexcept
    {
      return segments_ == other.segments_;
    }

    /**
     * @brief Compares two key paths for inequality.
     *
     * @param other Other key path.
     * @return true if paths differ.
     */
    [[nodiscard]] bool operator!=(const KeyPath &other) const noexcept
    {
      return !(*this == other);
    }

    /**
     * @brief Lexicographical ordering.
     *
     * Useful for deterministic tests and ordered containers.
     *
     * @param other Other key path.
     * @return true if this path is lexicographically smaller.
     */
    [[nodiscard]] bool operator<(const KeyPath &other) const noexcept
    {
      return segments_ < other.segments_;
    }

  private:
    Container segments_{};
  };

} // namespace vix::kv::keys

#endif // VIX_KV_KEYS_KEY_PATH_HPP
