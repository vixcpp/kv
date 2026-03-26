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
 */
#ifndef VIX_KV_KEYS_KEY_PATH_HPP
#define VIX_KV_KEYS_KEY_PATH_HPP

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace vix::kv::keys
{
  /**
   * @brief Structured hierarchical key path for Vix KV.
   *
   * A key path is made of ordered string segments such as:
   *
   * {"users", "42", "profile"}
   *
   * This structured form is the public developer-facing representation.
   * It will later be encoded into a flat store key compatible with
   * Softadastra Store.
   */
  class KeyPath
  {
  public:
    using Segment = std::string;
    using Container = std::vector<Segment>;

    /**
     * @brief Construct an empty key path.
     */
    KeyPath() = default;

    /**
     * @brief Construct a key path from an initializer list.
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
     * @brief Construct a key path from a vector of segments.
     *
     * @param parts Ordered key segments.
     */
    explicit KeyPath(Container parts)
        : segments_(std::move(parts))
    {
    }

    /**
     * @brief Append one segment to the key path.
     *
     * @param part Segment to append.
     * @return Reference to this key path.
     */
    KeyPath &push_back(std::string_view part)
    {
      segments_.emplace_back(part);
      return *this;
    }

    /**
     * @brief Return the number of segments.
     *
     * @return Segment count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return segments_.size();
    }

    /**
     * @brief Check whether the key path is empty.
     *
     * @return True if the key path has no segment.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return segments_.empty();
    }

    /**
     * @brief Access a segment by index.
     *
     * @param index Segment index.
     * @return Constant reference to the segment.
     */
    [[nodiscard]] const Segment &at(std::size_t index) const
    {
      return segments_.at(index);
    }

    /**
     * @brief Access the first segment.
     *
     * @return Constant reference to the first segment.
     */
    [[nodiscard]] const Segment &front() const
    {
      return segments_.front();
    }

    /**
     * @brief Access the last segment.
     *
     * @return Constant reference to the last segment.
     */
    [[nodiscard]] const Segment &back() const
    {
      return segments_.back();
    }

    /**
     * @brief Get all segments.
     *
     * @return Constant reference to the internal segment container.
     */
    [[nodiscard]] const Container &segments() const noexcept
    {
      return segments_;
    }

    /**
     * @brief Remove all segments.
     */
    void clear() noexcept
    {
      segments_.clear();
    }

    /**
     * @brief Compare two key paths for equality.
     *
     * @param other Another key path.
     * @return True if both key paths contain the same segments.
     */
    [[nodiscard]] bool operator==(const KeyPath &other) const noexcept
    {
      return segments_ == other.segments_;
    }

    /**
     * @brief Compare two key paths for inequality.
     *
     * @param other Another key path.
     * @return True if key paths differ.
     */
    [[nodiscard]] bool operator!=(const KeyPath &other) const noexcept
    {
      return !(*this == other);
    }

    /**
     * @brief Lexicographical ordering.
     *
     * Useful for deterministic comparisons in tests or ordered containers.
     *
     * @param other Another key path.
     * @return True if this key path is lexicographically smaller.
     */
    [[nodiscard]] bool operator<(const KeyPath &other) const noexcept
    {
      return segments_ < other.segments_;
    }

  private:
    Container segments_;
  };

} // namespace vix::kv::keys

#endif
