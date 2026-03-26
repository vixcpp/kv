/**
 *
 *  @file KvValue.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#ifndef VIX_KV_VALUES_KV_VALUE_HPP
#define VIX_KV_VALUES_KV_VALUE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vix::kv::values
{
  /**
   * @brief Developer-facing value wrapper for Vix KV.
   *
   * Internally, Softadastra Store uses a binary-safe container
   * (std::vector<uint8_t>). KvValue provides a convenient and
   * ergonomic abstraction on top of it.
   */
  class KvValue
  {
  public:
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Default constructor (empty value).
     */
    KvValue() = default;

    /**
     * @brief Construct from raw bytes.
     */
    explicit KvValue(Bytes data)
        : data_(std::move(data))
    {
    }

    /**
     * @brief Construct from string.
     *
     * The string is copied as raw bytes.
     */
    explicit KvValue(std::string_view str)
        : data_(str.begin(), str.end())
    {
    }

    /**
     * @brief Create a value from bytes.
     */
    [[nodiscard]] static KvValue from_bytes(Bytes data)
    {
      return KvValue(std::move(data));
    }

    /**
     * @brief Create a value from string.
     */
    [[nodiscard]] static KvValue from_string(std::string_view str)
    {
      return KvValue(str);
    }

    /**
     * @brief Access raw bytes.
     */
    [[nodiscard]] const Bytes &bytes() const noexcept
    {
      return data_;
    }

    /**
     * @brief Mutable access to raw bytes.
     */
    [[nodiscard]] Bytes &bytes() noexcept
    {
      return data_;
    }

    /**
     * @brief Convert value to string.
     *
     * This assumes the underlying data is valid UTF-8 / text.
     */
    [[nodiscard]] std::string to_string() const
    {
      return std::string(data_.begin(), data_.end());
    }

    /**
     * @brief Check if value is empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return data_.empty();
    }

    /**
     * @brief Size in bytes.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return data_.size();
    }

    /**
     * @brief Clear value.
     */
    void clear() noexcept
    {
      data_.clear();
    }

    /**
     * @brief Equality comparison.
     */
    [[nodiscard]] bool operator==(const KvValue &other) const noexcept
    {
      return data_ == other.data_;
    }

    /**
     * @brief Inequality comparison.
     */
    [[nodiscard]] bool operator!=(const KvValue &other) const noexcept
    {
      return !(*this == other);
    }

  private:
    Bytes data_;
  };

} // namespace vix::kv::values

#endif
