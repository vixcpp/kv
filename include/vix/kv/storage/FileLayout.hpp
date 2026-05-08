/**
 *
 *  @file FileLayout.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Storage file layout helpers
 *
 */

#ifndef VIX_KV_STORAGE_FILE_LAYOUT_HPP
#define VIX_KV_STORAGE_FILE_LAYOUT_HPP

#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include <vix/kv/core/KvConfig.hpp>

namespace vix::kv::storage
{
  namespace core = vix::kv::core;

  /**
   * @brief Central helper for KV storage paths.
   *
   * FileLayout defines where Vix KV stores its files.
   *
   * Default layout:
   *
   * @code
   * <path>/
   *   CURRENT
   *   MANIFEST
   *   wal/
   *     current.wal
   *   segments/
   *     segment-000000000000000001.kvseg
   *   snapshots/
   *     snapshot-000000000000000001.kvsnap
   * @endcode
   */
  class FileLayout
  {
  public:
    /**
     * @brief Segment file extension.
     */
    static constexpr const char *segment_extension = ".kvseg";

    /**
     * @brief Snapshot file extension.
     */
    static constexpr const char *snapshot_extension = ".kvsnap";

    /**
     * @brief Temporary file extension.
     */
    static constexpr const char *temporary_extension = ".tmp";

    /**
     * @brief Current WAL file name.
     */
    static constexpr const char *current_wal_name = "current.wal";

    /**
     * @brief Manifest file name.
     */
    static constexpr const char *manifest_name = "MANIFEST";

    /**
     * @brief CURRENT file name.
     */
    static constexpr const char *current_name = "CURRENT";

    /**
     * @brief WAL directory name.
     */
    static constexpr const char *wal_dir_name = "wal";

    /**
     * @brief Segment directory name.
     */
    static constexpr const char *segments_dir_name = "segments";

    /**
     * @brief Snapshot directory name.
     */
    static constexpr const char *snapshots_dir_name = "snapshots";

    /**
     * @brief Returns database root path.
     *
     * @param config KV config.
     * @return Database root path.
     */
    [[nodiscard]] static std::filesystem::path root(
        const core::KvConfig &config)
    {
      return config.path;
    }

    /**
     * @brief Returns data directory.
     *
     * @param config KV config.
     * @return Data directory.
     */
    [[nodiscard]] static std::filesystem::path data_dir(
        const core::KvConfig &config)
    {
      if (!config.data_dir.empty())
      {
        return config.data_dir;
      }

      return config.path;
    }

    /**
     * @brief Returns WAL directory.
     *
     * @param config KV config.
     * @return WAL directory.
     */
    [[nodiscard]] static std::filesystem::path wal_dir(
        const core::KvConfig &config)
    {
      if (!config.wal_dir.empty())
      {
        return config.wal_dir;
      }

      return data_dir(config) / wal_dir_name;
    }

    /**
     * @brief Returns segments directory.
     *
     * @param config KV config.
     * @return Segments directory.
     */
    [[nodiscard]] static std::filesystem::path segments_dir(
        const core::KvConfig &config)
    {
      if (!config.segments_dir.empty())
      {
        return config.segments_dir;
      }

      return data_dir(config) / segments_dir_name;
    }

    /**
     * @brief Returns snapshots directory.
     *
     * @param config KV config.
     * @return Snapshots directory.
     */
    [[nodiscard]] static std::filesystem::path snapshots_dir(
        const core::KvConfig &config)
    {
      if (!config.snapshots_dir.empty())
      {
        return config.snapshots_dir;
      }

      return data_dir(config) / snapshots_dir_name;
    }

    /**
     * @brief Returns current WAL path.
     *
     * @param config KV config.
     * @return Current WAL path.
     */
    [[nodiscard]] static std::filesystem::path wal_path(
        const core::KvConfig &config)
    {
      if (!config.wal_path.empty())
      {
        return config.wal_path;
      }

      return wal_dir(config) / current_wal_name;
    }

    /**
     * @brief Returns manifest path.
     *
     * @param config KV config.
     * @return Manifest path.
     */
    [[nodiscard]] static std::filesystem::path manifest_path(
        const core::KvConfig &config)
    {
      if (!config.manifest_path.empty())
      {
        return config.manifest_path;
      }

      return data_dir(config) / manifest_name;
    }

    /**
     * @brief Returns CURRENT path.
     *
     * @param config KV config.
     * @return CURRENT path.
     */
    [[nodiscard]] static std::filesystem::path current_path(
        const core::KvConfig &config)
    {
      if (!config.current_path.empty())
      {
        return config.current_path;
      }

      return data_dir(config) / current_name;
    }

    /**
     * @brief Returns a segment file path.
     *
     * @param config KV config.
     * @param segment_id Segment identifier.
     * @return Segment file path.
     */
    [[nodiscard]] static std::filesystem::path segment_path(
        const core::KvConfig &config,
        std::uint64_t segment_id)
    {
      return segments_dir(config) / segment_file_name(segment_id);
    }

    /**
     * @brief Returns a snapshot file path.
     *
     * @param config KV config.
     * @param snapshot_id Snapshot identifier.
     * @return Snapshot file path.
     */
    [[nodiscard]] static std::filesystem::path snapshot_path(
        const core::KvConfig &config,
        std::uint64_t snapshot_id)
    {
      return snapshots_dir(config) / snapshot_file_name(snapshot_id);
    }

    /**
     * @brief Returns a temporary path for a final path.
     *
     * @param final_path Final file path.
     * @return Temporary file path.
     */
    [[nodiscard]] static std::filesystem::path temporary_path(
        const std::filesystem::path &final_path)
    {
      return final_path.string() + temporary_extension;
    }

    /**
     * @brief Builds a segment file name.
     *
     * @param segment_id Segment identifier.
     * @return Segment file name.
     */
    [[nodiscard]] static std::string segment_file_name(
        std::uint64_t segment_id)
    {
      return numbered_file_name(
          "segment",
          segment_id,
          segment_extension);
    }

    /**
     * @brief Builds a snapshot file name.
     *
     * @param snapshot_id Snapshot identifier.
     * @return Snapshot file name.
     */
    [[nodiscard]] static std::string snapshot_file_name(
        std::uint64_t snapshot_id)
    {
      return numbered_file_name(
          "snapshot",
          snapshot_id,
          snapshot_extension);
    }

    /**
     * @brief Builds a deterministic numbered file name.
     *
     * @param prefix File prefix.
     * @param id Numeric identifier.
     * @param extension File extension.
     * @return File name.
     */
    [[nodiscard]] static std::string numbered_file_name(
        const std::string &prefix,
        std::uint64_t id,
        const std::string &extension)
    {
      std::ostringstream out;

      out << prefix
          << "-"
          << std::setw(18)
          << std::setfill('0')
          << id
          << extension;

      return out.str();
    }
  };

} // namespace vix::kv::storage

#endif // VIX_KV_STORAGE_FILE_LAYOUT_HPP
