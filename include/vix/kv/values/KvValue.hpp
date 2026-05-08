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
 *
 *  Binary-safe KV value
 *
 */

#ifndef VIX_KV_VALUES_KV_VALUE_HPP
#define VIX_KV_VALUES_KV_VALUE_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::kv::values
{
  /**
   * @brief Developer-facing value wrapper for Vix KV.
   *
   * KvValue stores raw bytes.
   *
   * Rules:
   * - values are binary-safe.
   * - empty values are allowed.
   * - the KV engine does not interpret value bytes.
   * - higher-level code decides what the bytes represent.
   */
  class KvValue
  {
  public:
    /**
     * @brief Internal byte container type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates an empty value.
     */
    KvValue() = default;

    /**
     * @brief Creates a value from raw bytes.
     *
     * @param data Value bytes.
     */
    explicit KvValue(Bytes data)
        : data_(std::move(data))
    {
    }

    /**
     * @brief Creates a value from a byte span.
     *
     * @param data Value bytes.
     */
    explicit KvValue(std::span<const std::uint8_t> data)
        : data_(data.begin(), data.end())
    {
    }

    /**
     * @brief Creates a value from a string view.
     *
     * The string is copied as raw bytes.
     *
     * @param text Text or binary-compatible bytes.
     */
    explicit KvValue(std::string_view text)
        : data_(text.begin(), text.end())
    {
    }

    /**
     * @brief Creates a value from raw bytes.
     *
     * @param data Value bytes.
     * @return KvValue instance.
     */
    [[nodiscard]] static KvValue from_bytes(Bytes data)
    {
      return KvValue(std::move(data));
    }

    /**
     * @brief Creates a value from a byte span.
     *
     * @param data Value bytes.
     * @return KvValue instance.
     */
    [[nodiscard]] static KvValue from_bytes(
        std::span<const std::uint8_t> data)
    {
      return KvValue(data);
    }

    /**
     * @brief Creates a value from a string view.
     *
     * @param text Text value.
     * @return KvValue instance.
     */
    [[nodiscard]] static KvValue from_string(std::string_view text)
    {
      return KvValue(text);
    }

    /**
     * @brief Returns immutable raw bytes.
     *
     * @return Read-only byte container.
     */
    [[nodiscard]] const Bytes &bytes() const noexcept
    {
      return data_;
    }

    /**
     * @brief Returns mutable raw bytes.
     *
     * @return Mutable byte container.
     */
    [[nodiscard]] Bytes &bytes() noexcept
    {
      return data_;
    }

    /**
     * @brief Returns immutable raw bytes.
     *
     * Alias for bytes().
     *
     * @return Read-only byte container.
     */
    [[nodiscard]] const Bytes &data() const noexcept
    {
      return data_;
    }

    /**
     * @brief Returns mutable raw bytes.
     *
     * Alias for bytes().
     *
     * @return Mutable byte container.
     */
    [[nodiscard]] Bytes &data() noexcept
    {
      return data_;
    }

    /**
     * @brief Returns a byte span over the value.
     *
     * @return Read-only byte span.
     */
    [[nodiscard]] std::span<const std::uint8_t> span() const noexcept
    {
      return std::span<const std::uint8_t>(data_.data(), data_.size());
    }

    /**
     * @brief Converts value bytes to a string.
     *
     * This does not validate UTF-8.
     *
     * @return String built from stored bytes.
     */
    [[nodiscard]] std::string to_string() const
    {
      return std::string(data_.begin(), data_.end());
    }

    /**
     * @brief Returns true if the value contains no bytes.
     *
     * @return true when empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return data_.empty();
    }

    /**
     * @brief Returns value size in bytes.
     *
     * @return Byte count.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
      return data_.size();
    }

    /**
     * @brief Reserves storage for bytes.
     *
     * @param capacity Requested capacity.
     */
    void reserve(std::size_t capacity)
    {
      data_.reserve(capacity);
    }

    /**
     * @brief Clears all bytes.
     */
    void clear() noexcept
    {
      data_.clear();
    }

    /**
     * @brief Appends one byte.
     *
     * @param byte Byte to append.
     */
    void push_back(std::uint8_t byte)
    {
      data_.push_back(byte);
    }

    /**
     * @brief Appends bytes from a span.
     *
     * @param data Bytes to append.
     */
    void append(std::span<const std::uint8_t> data)
    {
      data_.insert(data_.end(), data.begin(), data.end());
    }

    /**
     * @brief Appends text as raw bytes.
     *
     * @param text Text bytes to append.
     */
    void append(std::string_view text)
    {
      data_.insert(data_.end(), text.begin(), text.end());
    }

    /**
     * @brief Compares two values for equality.
     *
     * @param other Other value.
     * @return true if bytes are equal.
     */
    [[nodiscard]] bool operator==(const KvValue &other) const noexcept
    {
      return data_ == other.data_;
    }

    /**
     * @brief Compares two values for inequality.
     *
     * @param other Other value.
     * @return true if bytes differ.
     */
    [[nodiscard]] bool operator!=(const KvValue &other) const noexcept
    {
      return !(*this == other);
    }

  private:
    Bytes data_{};
  };

} // namespace vix::kv::values

#endif // VIX_KV_VALUES_KV_VALUE_HPP
