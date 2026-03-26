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
 */
#ifndef VIX_KV_API_KV_HPP
#define VIX_KV_API_KV_HPP

#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <filesystem>

#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/internal/KvEngine.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace vix::kv::api
{
  namespace keys = vix::kv::keys;
  namespace values = vix::kv::values;
  namespace internal = vix::kv::internal;

  /**
   * @brief Public KV interface.
   *
   * Thin, developer-friendly façade over KvEngine.
   *
   * Usage:
   * @code
   * auto kv = vix::kv::api::Kv::open({});
   * kv.set({"users", "42"}, "Alice");
   * auto v = kv.get({"users", "42"});
   * @endcode
   */
  class Kv
  {
  public:
    /**
     * @brief Open a KV instance with options.
     *
     * This initializes the underlying engine and performs WAL recovery.
     */
    static Kv open(const KvOptions &options)
    {
      auto config = options.to_store_config();

      const std::filesystem::path wal_path(config.wal_path);
      if (wal_path.has_parent_path())
      {
        std::filesystem::create_directories(wal_path.parent_path());
      }

      internal::KvEngine engine(std::move(config));
      engine.open();
      return Kv(std::move(engine));
    }

    /**
     * @brief Default destructor.
     */
    ~Kv() = default;

    /**
     * @brief Insert or update a value.
     */
    bool set(const keys::KeyPath &key,
             const values::KvValue &value)
    {
      return engine_->set(key, value);
    }

    /**
     * @brief Get a value.
     */
    std::optional<values::KvValue>
    get(const keys::KeyPath &key) const
    {
      return engine_->get(key);
    }

    /**
     * @brief Remove a key.
     */
    bool erase(const keys::KeyPath &key)
    {
      return engine_->erase(key);
    }

    /**
     * @brief List entries by prefix.
     */
    std::vector<std::pair<keys::KeyPath, values::KvValue>>
    list(const keys::KeyPath &prefix) const
    {
      return engine_->list(prefix);
    }

  private:
    /**
     * @brief Private constructor.
     */
    explicit Kv(internal::KvEngine engine)
        : engine_(std::make_unique<internal::KvEngine>(std::move(engine)))
    {
    }

  private:
    std::unique_ptr<internal::KvEngine> engine_;
  };

} // namespace vix::kv::api

#endif
