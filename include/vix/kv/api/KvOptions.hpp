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
 *
 *  Public KV options
 *
 */

#ifndef VIX_KV_API_KV_OPTIONS_HPP
#define VIX_KV_API_KV_OPTIONS_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvLimits.hpp>

namespace vix::kv::api
{
  namespace core = vix::kv::core;

  /**
   * @brief Public options used to open a Vix KV database.
   *
   * KvOptions is developer-facing.
   *
   * It is converted to core::KvConfig before reaching the internal engine.
   *
   * Example:
   * @code
   * vix::kv::api::KvOptions options;
   * options.path = "data/app.kv";
   * options.auto_flush = true;
   *
   * auto db = vix::kv::open(options);
   * @endcode
   */
  struct KvOptions
  {
    /**
     * @brief Root directory of the KV database.
     *
     * Example:
     * data/app.kv
     */
    std::filesystem::path path{"data/kv"};

    /**
     * @brief Enable WAL-backed durability.
     */
    bool enable_wal{true};

    /**
     * @brief Flush WAL after each accepted write.
     */
    bool auto_flush{true};

    /**
     * @brief Create database directories automatically.
     */
    bool create_directories{true};

    /**
     * @brief Open database in read-only mode.
     */
    bool read_only{false};

    /**
     * @brief Recover WAL during open().
     */
    bool recover_on_open{true};

    /**
     * @brief Use stronger sync behavior for file writes when supported.
     */
    bool sync_writes{true};

    /**
     * @brief Enable record checksums.
     */
    bool enable_checksums{true};

    /**
     * @brief Enable automatic compaction when supported.
     */
    bool enable_auto_compaction{false};

    /**
     * @brief Initial in-memory table capacity.
     */
    std::size_t initial_capacity{core::KvLimits::default_initial_capacity};

    /**
     * @brief Maximum number of key path segments.
     */
    std::size_t max_key_segments{core::KvLimits::max_key_segments};

    /**
     * @brief Maximum size of one key segment.
     */
    std::size_t max_key_segment_size{
        core::KvLimits::max_key_segment_size};

    /**
     * @brief Maximum encoded key size.
     */
    std::size_t max_key_size{core::KvLimits::max_key_size};

    /**
     * @brief Maximum value size.
     */
    std::size_t max_value_size{core::KvLimits::max_value_size};

    /**
     * @brief Maximum record size.
     */
    std::size_t max_record_size{core::KvLimits::max_record_size};

    /**
     * @brief Maximum WAL size before rotation, if supported.
     */
    std::uint64_t max_wal_size{core::KvLimits::default_max_wal_size};

    /**
     * @brief Target segment size.
     */
    std::uint64_t segment_size{core::KvLimits::default_segment_size};

    /**
     * @brief Maximum number of open files.
     */
    std::size_t max_open_files{core::KvLimits::default_max_open_files};

    /**
     * @brief Number of records processed per recovery batch.
     */
    std::size_t recovery_batch_size{
        core::KvLimits::default_recovery_batch_size};

    /**
     * @brief Creates default options.
     */
    KvOptions() = default;

    /**
     * @brief Creates options with a custom path.
     *
     * @param database_path Database root directory.
     */
    explicit KvOptions(std::filesystem::path database_path)
        : path(std::move(database_path))
    {
    }

    /**
     * @brief Creates durable production-oriented options.
     *
     * @param database_path Database root directory.
     * @return Durable options.
     */
    [[nodiscard]] static KvOptions durable(
        std::filesystem::path database_path)
    {
      KvOptions options(std::move(database_path));

      options.enable_wal = true;
      options.auto_flush = true;
      options.create_directories = true;
      options.read_only = false;
      options.recover_on_open = true;
      options.sync_writes = true;
      options.enable_checksums = true;
      options.enable_auto_compaction = false;

      return options;
    }

    /**
     * @brief Creates faster options for tests and benchmarks.
     *
     * @param database_path Database root directory.
     * @return Fast options.
     */
    [[nodiscard]] static KvOptions fast(
        std::filesystem::path database_path)
    {
      KvOptions options(std::move(database_path));

      options.enable_wal = true;
      options.auto_flush = false;
      options.create_directories = true;
      options.read_only = false;
      options.recover_on_open = true;
      options.sync_writes = false;
      options.enable_checksums = true;
      options.enable_auto_compaction = false;

      return options;
    }

    /**
     * @brief Creates memory-only options.
     *
     * Memory-only mode does not persist data and does not recover data
     * after restart.
     *
     * @return Memory-only options.
     */
    [[nodiscard]] static KvOptions memory_only()
    {
      KvOptions options;

      options.path.clear();
      options.enable_wal = false;
      options.auto_flush = false;
      options.create_directories = false;
      options.read_only = false;
      options.recover_on_open = false;
      options.sync_writes = false;
      options.enable_checksums = false;
      options.enable_auto_compaction = false;

      return options;
    }

    /**
     * @brief Converts public options to internal engine configuration.
     *
     * @return Normalized internal configuration.
     */
    [[nodiscard]] core::KvConfig to_config() const
    {
      core::KvConfig config;

      config.path = path;
      config.normalize_paths();

      config.enable_wal = enable_wal;
      config.auto_flush = auto_flush;
      config.create_directories = create_directories;
      config.read_only = read_only;
      config.recover_on_open = recover_on_open;
      config.sync_writes = sync_writes;
      config.enable_checksums = enable_checksums;
      config.enable_auto_compaction = enable_auto_compaction;

      config.initial_capacity = initial_capacity;
      config.max_key_segments = max_key_segments;
      config.max_key_segment_size = max_key_segment_size;
      config.max_key_size = max_key_size;
      config.max_value_size = max_value_size;
      config.max_record_size = max_record_size;
      config.max_wal_size = max_wal_size;
      config.segment_size = segment_size;
      config.max_open_files = max_open_files;
      config.recovery_batch_size = recovery_batch_size;

      if (!enable_wal)
      {
        config.wal_path.clear();
      }

      if (!recover_on_open && !enable_wal && path.empty())
      {
        return core::KvConfig::memory_only();
      }

      return config;
    }

    /**
     * @brief Validates options by converting them to KvConfig.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] core::KvResult<void> validate() const
    {
      return to_config().validate();
    }

    /**
     * @brief Returns true if the options are valid.
     *
     * @return true when validate() succeeds.
     */
    [[nodiscard]] bool is_valid() const
    {
      return validate().is_ok();
    }
  };

} // namespace vix::kv::api

#endif // VIX_KV_API_KV_OPTIONS_HPP
