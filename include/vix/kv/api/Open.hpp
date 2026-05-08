/**
 *
 *  @file Open.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public open helpers
 *
 */

#ifndef VIX_KV_API_OPEN_HPP
#define VIX_KV_API_OPEN_HPP

#include <vix/kv/api/Kv.hpp>
#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/core/KvResult.hpp>

namespace vix::kv
{
  namespace api = vix::kv::api;
  namespace core = vix::kv::core;

  /**
   * @brief Opens a KV database with default options.
   *
   * This is the simplest way to create a Vix KV database.
   *
   * Example:
   * @code
   * auto db = vix::kv::open();
   * @endcode
   *
   * @return KV handle or KvError.
   */
  [[nodiscard]] core::KvResult<api::Kv> open();

  /**
   * @brief Opens a KV database with custom options.
   *
   * Example:
   * @code
   * vix::kv::api::KvOptions options =
   *     vix::kv::api::KvOptions::durable("data/app.kv");
   *
   * auto db = vix::kv::open(options);
   * @endcode
   *
   * @param options Public KV options.
   * @return KV handle or KvError.
   */
  [[nodiscard]] core::KvResult<api::Kv> open(
      const api::KvOptions &options);

  /**
   * @brief Opens a memory-only KV database.
   *
   * Memory-only mode does not persist data after restart.
   *
   * @return KV handle or KvError.
   */
  [[nodiscard]] core::KvResult<api::Kv> open_memory();

  /**
   * @brief Opens a durable KV database at a path.
   *
   * This enables WAL persistence and safe auto flush.
   *
   * @param path Database root path.
   * @return KV handle or KvError.
   */
  [[nodiscard]] core::KvResult<api::Kv> open_durable(
      std::filesystem::path path);

  /**
   * @brief Opens a fast KV database at a path.
   *
   * This keeps WAL enabled but disables auto flush.
   * It is useful for tests, benchmarks, and temporary workloads.
   *
   * @param path Database root path.
   * @return KV handle or KvError.
   */
  [[nodiscard]] core::KvResult<api::Kv> open_fast(
      std::filesystem::path path);

} // namespace vix::kv

#endif // VIX_KV_API_OPEN_HPP
