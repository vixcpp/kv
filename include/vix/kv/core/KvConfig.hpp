/**
 *
 *  @file KvConfig.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Internal KV engine configuration
 *
 */

#ifndef VIX_KV_CORE_KV_CONFIG_HPP
#define VIX_KV_CORE_KV_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/core/KvResult.hpp>

namespace vix::kv::core
{
  /**
   * @brief Internal configuration used by the Vix KV engine.
   *
   * KvConfig is the normalized engine configuration.
   *
   * Public APIs can expose friendlier options, but the engine should only
   * depend on this structure.
   *
   * Rules:
   * - path must not be empty.
   * - WAL path must not be empty when WAL is enabled.
   * - data directory must not be empty.
   * - limits must be greater than zero.
   * - each database should have its own directory.
   */
  struct KvConfig
  {
    /**
     * @brief Root directory of the KV database.
     *
     * Example:
     * data/app.kv
     */
    std::filesystem::path path{"data/kv"};

    /**
     * @brief Directory containing KV data files.
     *
     * Usually derived from path.
     */
    std::filesystem::path data_dir{"data/kv"};

    /**
     * @brief Directory containing WAL files.
     *
     * Usually derived from data_dir.
     */
    std::filesystem::path wal_dir{"data/kv/wal"};

    /**
     * @brief Directory containing segment files.
     *
     * Usually derived from data_dir.
     */
    std::filesystem::path segments_dir{"data/kv/segments"};

    /**
     * @brief Directory containing snapshot files.
     *
     * Usually derived from data_dir.
     */
    std::filesystem::path snapshots_dir{"data/kv/snapshots"};

    /**
     * @brief Current WAL file path.
     */
    std::filesystem::path wal_path{"data/kv/wal/current.wal"};

    /**
     * @brief Manifest file path.
     */
    std::filesystem::path manifest_path{"data/kv/MANIFEST"};

    /**
     * @brief CURRENT file path.
     */
    std::filesystem::path current_path{"data/kv/CURRENT"};

    /**
     * @brief Enable WAL persistence.
     *
     * When true, mutations are appended to WAL before being applied.
     */
    bool enable_wal{true};

    /**
     * @brief Flush WAL after each accepted mutation.
     *
     * This is the safest default.
     */
    bool auto_flush{true};

    /**
     * @brief Create directories automatically during open().
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
     * @brief Sync file writes more aggressively when supported.
     */
    bool sync_writes{true};

    /**
     * @brief Enable checksums for records.
     */
    bool enable_checksums{true};

    /**
     * @brief Enable automatic compaction when supported.
     */
    bool enable_auto_compaction{false};

    /**
     * @brief Initial in-memory table capacity.
     */
    std::size_t initial_capacity{KvLimits::default_initial_capacity};

    /**
     * @brief Maximum number of key path segments.
     */
    std::size_t max_key_segments{KvLimits::max_key_segments};

    /**
     * @brief Maximum size of one key segment.
     */
    std::size_t max_key_segment_size{KvLimits::max_key_segment_size};

    /**
     * @brief Maximum encoded key size.
     */
    std::size_t max_key_size{KvLimits::max_key_size};

    /**
     * @brief Maximum value size.
     */
    std::size_t max_value_size{KvLimits::max_value_size};

    /**
     * @brief Maximum record size.
     */
    std::size_t max_record_size{KvLimits::max_record_size};

    /**
     * @brief Maximum WAL size before rotation, if supported.
     */
    std::uint64_t max_wal_size{KvLimits::default_max_wal_size};

    /**
     * @brief Target segment size.
     */
    std::uint64_t segment_size{KvLimits::default_segment_size};

    /**
     * @brief Maximum number of open files.
     */
    std::size_t max_open_files{KvLimits::default_max_open_files};

    /**
     * @brief Recovery batch size.
     */
    std::size_t recovery_batch_size{KvLimits::default_recovery_batch_size};

    /**
     * @brief Creates a default configuration.
     */
    KvConfig() = default;

    /**
     * @brief Creates a configuration from a root path.
     *
     * @param root_path Database root path.
     */
    explicit KvConfig(std::filesystem::path root_path)
        : path(std::move(root_path))
    {
      normalize_paths();
    }

    /**
     * @brief Creates a production-oriented durable configuration.
     *
     * @param root_path Database root path.
     * @return Durable KV configuration.
     */
    [[nodiscard]] static KvConfig durable(std::filesystem::path root_path)
    {
      KvConfig config(std::move(root_path));

      config.enable_wal = true;
      config.auto_flush = true;
      config.create_directories = true;
      config.recover_on_open = true;
      config.sync_writes = true;
      config.enable_checksums = true;
      config.enable_auto_compaction = false;

      config.normalize_paths();

      return config;
    }

    /**
     * @brief Creates a fast configuration for benchmarks and tests.
     *
     * @param root_path Database root path.
     * @return Fast KV configuration.
     */
    [[nodiscard]] static KvConfig fast(std::filesystem::path root_path)
    {
      KvConfig config(std::move(root_path));

      config.enable_wal = true;
      config.auto_flush = false;
      config.create_directories = true;
      config.recover_on_open = true;
      config.sync_writes = false;
      config.enable_checksums = true;
      config.enable_auto_compaction = false;

      config.normalize_paths();

      return config;
    }

    /**
     * @brief Creates an in-memory configuration.
     *
     * WAL is disabled and no durable recovery is expected.
     *
     * @return Memory-only KV configuration.
     */
    [[nodiscard]] static KvConfig memory_only()
    {
      KvConfig config;

      config.path.clear();
      config.data_dir.clear();
      config.wal_dir.clear();
      config.segments_dir.clear();
      config.snapshots_dir.clear();
      config.wal_path.clear();
      config.manifest_path.clear();
      config.current_path.clear();

      config.enable_wal = false;
      config.auto_flush = false;
      config.create_directories = false;
      config.read_only = false;
      config.recover_on_open = false;
      config.sync_writes = false;
      config.enable_checksums = false;
      config.enable_auto_compaction = false;

      return config;
    }

    /**
     * @brief Normalizes all derived paths from the root path.
     */
    void normalize_paths()
    {
      if (path.empty())
      {
        return;
      }

      data_dir = path;
      wal_dir = data_dir / "wal";
      segments_dir = data_dir / "segments";
      snapshots_dir = data_dir / "snapshots";

      wal_path = wal_dir / "current.wal";
      manifest_path = data_dir / "MANIFEST";
      current_path = data_dir / "CURRENT";
    }

    /**
     * @brief Returns true if this is a memory-only configuration.
     *
     * @return true when WAL and recovery are disabled and path is empty.
     */
    [[nodiscard]] bool is_memory_only() const noexcept
    {
      return !enable_wal && !recover_on_open && path.empty();
    }

    /**
     * @brief Validates this configuration.
     *
     * @return Success or KvError.
     */
    [[nodiscard]] KvResult<void> validate() const
    {
      if (!is_memory_only() && path.empty())
      {
        return KvResult<void>::err(
            KvError::config("KV path must not be empty"));
      }

      if (!is_memory_only() && data_dir.empty())
      {
        return KvResult<void>::err(
            KvError::config("KV data directory must not be empty"));
      }

      if (enable_wal && wal_path.empty())
      {
        return KvResult<void>::err(
            KvError::config("WAL path must not be empty when WAL is enabled"));
      }

      if (read_only && create_directories)
      {
        return KvResult<void>::err(
            KvError::config(
                "read-only mode cannot create directories automatically"));
      }

      if (!KvLimits::valid_initial_capacity(initial_capacity))
      {
        return KvResult<void>::err(
            KvError::config("initial capacity must be greater than zero"));
      }

      if (max_key_segments == 0)
      {
        return KvResult<void>::err(
            KvError::config("max key segments must be greater than zero"));
      }

      if (max_key_segment_size == 0)
      {
        return KvResult<void>::err(
            KvError::config("max key segment size must be greater than zero"));
      }

      if (!KvLimits::valid_key_size(max_key_size))
      {
        return KvResult<void>::err(
            KvError::config("max key size is invalid"));
      }

      if (max_value_size == 0)
      {
        return KvResult<void>::err(
            KvError::config("max value size must be greater than zero"));
      }

      if (!KvLimits::valid_record_size(max_record_size))
      {
        return KvResult<void>::err(
            KvError::config("max record size is invalid"));
      }

      if (!KvLimits::valid_wal_size(max_wal_size))
      {
        return KvResult<void>::err(
            KvError::config("max WAL size must be greater than zero"));
      }

      if (!KvLimits::valid_segment_size(segment_size))
      {
        return KvResult<void>::err(
            KvError::config("segment size must be greater than zero"));
      }

      if (max_open_files == 0)
      {
        return KvResult<void>::err(
            KvError::config("max open files must be greater than zero"));
      }

      if (recovery_batch_size == 0)
      {
        return KvResult<void>::err(
            KvError::config("recovery batch size must be greater than zero"));
      }

      if (max_key_segment_size > max_key_size)
      {
        return KvResult<void>::err(
            KvError::config(
                "max key segment size must not exceed max key size"));
      }

      if (max_record_size < max_key_size)
      {
        return KvResult<void>::err(
            KvError::config(
                "max record size must be greater than or equal to max key size"));
      }

      if (max_record_size < max_value_size)
      {
        return KvResult<void>::err(
            KvError::config(
                "max record size must be greater than or equal to max value size"));
      }

      return KvResult<void>::ok();
    }

    /**
     * @brief Returns true if the configuration is valid.
     *
     * @return true when validate() succeeds.
     */
    [[nodiscard]] bool is_valid() const
    {
      return validate().is_ok();
    }
  };

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_CONFIG_HPP
