/**
 *
 *  @file Kv.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public KV database API
 *
 */

#ifndef VIX_KV_API_KV_HPP
#define VIX_KV_API_KV_HPP

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/core/KvStats.hpp>
#include <vix/kv/internal/KvEngine.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace vix::kv::api
{
  namespace core = vix::kv::core;
  namespace internal = vix::kv::internal;
  namespace keys = vix::kv::keys;
  namespace values = vix::kv::values;

  /**
   * @brief Public Vix KV database handle.
   *
   * Kv is the developer-facing API.
   *
   * It wraps the internal KvEngine and exposes simple operations:
   * - open
   * - close
   * - set
   * - get
   * - erase
   * - contains
   * - list
   * - flush
   * - stats
   *
   * Rules:
   * - open() must be called before operations.
   * - set() is durable when WAL is enabled.
   * - get() returns NotFound when the key does not exist.
   * - erase() returns NotFound when the key does not exist.
   * - empty values are allowed.
   */
  class Kv
  {
  public:
    /**
     * @brief List result type.
     */
    using ListResult =
        std::vector<std::pair<keys::KeyPath, values::KvValue>>;

    /**
     * @brief Creates a closed KV handle with default options.
     */
    Kv();

    /**
     * @brief Creates a closed KV handle with options.
     *
     * @param options Public KV options.
     */
    explicit Kv(KvOptions options);

    /**
     * @brief Non-copyable.
     */
    Kv(const Kv &) = delete;

    /**
     * @brief Non-copyable.
     */
    Kv &operator=(const Kv &) = delete;

    /**
     * @brief Movable.
     */
    Kv(Kv &&other) noexcept;

    /**
     * @brief Movable.
     */
    Kv &operator=(Kv &&other) noexcept;

    /**
     * @brief Closes the KV handle.
     */
    ~Kv();

    /**
     * @brief Opens a KV database with default options.
     *
     * @return KV handle or KvError.
     */
    [[nodiscard]] static core::KvResult<Kv> open();

    /**
     * @brief Opens a KV database with custom options.
     *
     * @param options Public KV options.
     * @return KV handle or KvError.
     */
    [[nodiscard]] static core::KvResult<Kv> open(
        const KvOptions &options);

    /**
     * @brief Opens this KV handle.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> open_handle();

    /**
     * @brief Closes this KV handle.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> close();

    /**
     * @brief Flushes durable state.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> flush();

    /**
     * @brief Stores or replaces a value.
     *
     * @param key Public key path.
     * @param value KV value.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> set(
        const keys::KeyPath &key,
        const values::KvValue &value);

    /**
     * @brief Stores or replaces a string value.
     *
     * @param key Public key path.
     * @param value Text value.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> set(
        const keys::KeyPath &key,
        std::string_view value);

    /**
     * @brief Stores or replaces a string value using a simple text key.
     *
     * This convenience method throws std::runtime_error on failure.
     *
     * @param key Text key. Slash-separated keys are supported.
     * @param value Text value.
     */
    void put(
        std::string_view key,
        std::string_view value);

    /**
     * @brief Reads a string value using a simple text key.
     *
     * Returns std::nullopt when the key does not exist.
     * Throws std::runtime_error for other failures.
     *
     * @param key Text key. Slash-separated keys are supported.
     * @return Optional string value.
     */
    [[nodiscard]] std::optional<std::string> get(
        std::string_view key) const;

    /**
     * @brief Stores a string value using initializer-list key parts.
     *
     * This fixes calls like:
     * db.set({"users", "1", "name"}, "Ada");
     */
    [[nodiscard]] core::KvResult<void> set(
        std::initializer_list<std::string_view> key,
        std::string_view value);

    /**
     * @brief Reads a value using initializer-list key parts.
     */
    [[nodiscard]] core::KvResult<values::KvValue> get(
        std::initializer_list<std::string_view> key) const;

    /**
     * @brief Checks a key using initializer-list key parts.
     */
    [[nodiscard]] bool contains(
        std::initializer_list<std::string_view> key) const;

    /**
     * @brief Lists values using initializer-list prefix parts.
     */
    [[nodiscard]] core::KvResult<ListResult> list(
        std::initializer_list<std::string_view> prefix) const;

    /**
     * @brief Reads a value.
     *
     * @param key Public key path.
     * @return Value or KvError.
     */
    [[nodiscard]] core::KvResult<values::KvValue> get(
        const keys::KeyPath &key) const;

    /**
     * @brief Removes a key.
     *
     * @param key Public key path.
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> erase(
        const keys::KeyPath &key);

    /**
     * @brief Checks whether a key exists.
     *
     * @param key Public key path.
     * @return true if key exists.
     */
    [[nodiscard]] bool contains(const keys::KeyPath &key) const;

    /**
     * @brief Lists entries by prefix.
     *
     * Empty prefix lists all live entries.
     *
     * @param prefix Public prefix path.
     * @return List result or KvError.
     */
    [[nodiscard]] core::KvResult<ListResult> list(
        const keys::KeyPath &prefix = {}) const;

    /**
     * @brief Returns number of live keys.
     *
     * @return Live key count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns true if the database has no live keys.
     *
     * @return true when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns true if the handle is open.
     *
     * @return true when open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns public options.
     *
     * @return Options.
     */
    [[nodiscard]] const KvOptions &options() const noexcept;

    /**
     * @brief Returns runtime stats.
     *
     * @return Stats snapshot.
     */
    [[nodiscard]] core::KvStats stats() const;

  private:
    /**
     * @brief Creates a KV handle from options and an engine.
     *
     * @param options Public options.
     * @param engine Internal engine.
     */
    Kv(
        KvOptions options,
        internal::KvEngine engine);

    /**
     * @brief Returns success if engine exists.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> require_engine() const;

    /**
     * @brief Moves from another handle.
     *
     * @param other Source handle.
     */
    void move_from(Kv &&other) noexcept;

  private:
    KvOptions options_{};
    std::unique_ptr<internal::KvEngine> engine_{};

    [[nodiscard]] static keys::KeyPath make_key_path(
        std::string_view key);

    [[nodiscard]] static keys::KeyPath make_key_path(
        std::initializer_list<std::string_view> parts);
  };

} // namespace vix::kv::api

#endif // VIX_KV_API_KV_HPP
