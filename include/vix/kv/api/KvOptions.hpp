/**
 *
 *  @file KvOptions.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#ifndef VIX_KV_API_KV_OPTIONS_HPP
#define VIX_KV_API_KV_OPTIONS_HPP

#include <string>

#include <softadastra/store/core/StoreConfig.hpp>

namespace vix::kv::api
{
  namespace store_core = softadastra::store::core;

  /**
   * @brief Options used to open a KV database.
   *
   * This is a thin wrapper around Softadastra StoreConfig with
   * a developer-friendly API.
   */
  struct KvOptions
  {
    /**
     * @brief Path to WAL file.
     *
     * Example:
     * "./data/app.wal"
     */
    std::string path{"data/kv.wal"};

    /**
     * @brief Enable WAL persistence.
     */
    bool enable_wal{true};

    /**
     * @brief Auto flush WAL after each write.
     */
    bool auto_flush{true};

    /**
     * @brief Initial capacity for in-memory index.
     */
    std::size_t initial_capacity{1024};

    /**
     * @brief Convert to StoreConfig.
     */
    [[nodiscard]] store_core::StoreConfig to_store_config() const
    {
      store_core::StoreConfig config;
      config.wal_path = path;
      config.enable_wal = enable_wal;
      config.auto_flush = auto_flush;
      config.initial_capacity = initial_capacity;
      return config;
    }
  };

} // namespace vix::kv::api

#endif
