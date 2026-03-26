/**
 *
 *  @file KvEngine.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#ifndef VIX_KV_INTERNAL_KV_ENGINE_HPP
#define VIX_KV_INTERNAL_KV_ENGINE_HPP

#include <optional>
#include <string>
#include <vector>

#include <softadastra/store/core/StoreConfig.hpp>
#include <softadastra/store/engine/StoreEngine.hpp>
#include <softadastra/store/types/Key.hpp>
#include <softadastra/store/types/Value.hpp>

#include <vix/kv/keys/KeyEncoder.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/values/KvValue.hpp>
#include <vix/kv/values/ValueCodec.hpp>

namespace vix::kv::internal
{
  namespace store_core = softadastra::store::core;
  namespace store_engine = softadastra::store::engine;
  namespace store_types = softadastra::store::types;

  namespace keys = vix::kv::keys;
  namespace values = vix::kv::values;

  /**
   * @brief Internal KV engine built on top of Softadastra StoreEngine.
   *
   * Responsibilities:
   * - Encode KeyPath → store key
   * - Encode KvValue → store value
   * - Delegate operations to StoreEngine
   * - Provide higher-level KV semantics
   */
  class KvEngine
  {
  public:
    /**
     * @brief Construct KV engine with configuration.
     */
    explicit KvEngine(store_core::StoreConfig config);

    /**
     * @brief Open / initialize the engine.
     *
     * This will trigger WAL recovery inside StoreEngine.
     */
    void open();

    /**
     * @brief Insert or update a key.
     */
    bool set(const keys::KeyPath &key,
             const values::KvValue &value);

    /**
     * @brief Get a value by key.
     */
    std::optional<values::KvValue>
    get(const keys::KeyPath &key) const;

    /**
     * @brief Remove a key.
     */
    bool erase(const keys::KeyPath &key);

    /**
     * @brief List entries by prefix.
     *
     * Returns (KeyPath, KvValue) pairs.
     */
    std::vector<std::pair<keys::KeyPath, values::KvValue>>
    list(const keys::KeyPath &prefix) const;

  private:
    /**
     * @brief Convert store key string → KeyPath
     */
    static keys::KeyPath decode_key(const std::string &encoded);

  private:
    store_engine::StoreEngine store_;
  };

} // namespace vix::kv::internal

#endif
