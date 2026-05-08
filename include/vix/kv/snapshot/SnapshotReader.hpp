/**
 *
 *  @file SnapshotReader.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot reader
 *
 */

#ifndef VIX_KV_SNAPSHOT_SNAPSHOT_READER_HPP
#define VIX_KV_SNAPSHOT_SNAPSHOT_READER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/snapshot/Snapshot.hpp>
#include <vix/kv/storage/FileLayout.hpp>

namespace vix::kv::snapshot
{
  namespace core = vix::kv::core;
  namespace storage = vix::kv::storage;

  /**
   * @brief Reads KV snapshots from disk.
   *
   * SnapshotReader decodes files written by SnapshotWriter.
   *
   * It verifies:
   * - snapshot magic
   * - format version
   * - entry sizes
   * - checksum
   * - full buffer consumption
   */
  class SnapshotReader
  {
  public:
    /**
     * @brief Byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a reader from config.
     *
     * @param config KV configuration.
     */
    explicit SnapshotReader(core::KvConfig config);

    /**
     * @brief Creates a reader from explicit snapshot path.
     *
     * @param path Snapshot file path.
     */
    explicit SnapshotReader(std::filesystem::path path);

    /**
     * @brief Reads the configured snapshot path.
     *
     * @return Snapshot or KvError.
     */
    [[nodiscard]] core::KvResult<Snapshot> read() const;

    /**
     * @brief Reads a specific snapshot id from config paths.
     *
     * @param snapshot_id Snapshot identifier.
     * @return Snapshot or KvError.
     */
    [[nodiscard]] core::KvResult<Snapshot>
    read_snapshot(std::uint64_t snapshot_id) const;

    /**
     * @brief Reads a snapshot from a specific path.
     *
     * @param path Snapshot file path.
     * @return Snapshot or KvError.
     */
    [[nodiscard]] core::KvResult<Snapshot>
    read_from(const std::filesystem::path &path) const;

    /**
     * @brief Decodes snapshot bytes.
     *
     * @param bytes Snapshot bytes.
     * @return Snapshot or KvError.
     */
    [[nodiscard]] static core::KvResult<Snapshot>
    decode(const Bytes &bytes);

    /**
     * @brief Decodes snapshot bytes.
     *
     * @param bytes Snapshot bytes.
     * @return Snapshot or KvError.
     */
    [[nodiscard]] static core::KvResult<Snapshot>
    decode(std::span<const std::uint8_t> bytes);

    /**
     * @brief Returns reader config.
     *
     * @return KV config.
     */
    [[nodiscard]] const core::KvConfig &config() const noexcept;

    /**
     * @brief Returns configured path.
     *
     * @return Snapshot path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

  private:
    /**
     * @brief Reads an entire file into memory.
     *
     * @param path Snapshot path.
     * @return File bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    read_file(const std::filesystem::path &path);

    /**
     * @brief Verifies the trailing checksum.
     *
     * @param bytes Snapshot bytes.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    verify_checksum(std::span<const std::uint8_t> bytes);

    /**
     * @brief Reads and validates the snapshot header.
     *
     * @param bytes Snapshot bytes.
     * @param offset Current offset, advanced on success.
     * @param snapshot Output snapshot.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    decode_header(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        Snapshot &snapshot,
        std::uint64_t &entry_count);

    /**
     * @brief Decodes all entries.
     *
     * @param bytes Snapshot bytes.
     * @param offset Current offset, advanced on success.
     * @param entry_count Number of entries to decode.
     * @param snapshot Output snapshot.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    decode_entries(
        std::span<const std::uint8_t> bytes,
        std::size_t &offset,
        std::uint64_t entry_count,
        Snapshot &snapshot);

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
  };

} // namespace vix::kv::snapshot

#endif // VIX_KV_SNAPSHOT_SNAPSHOT_READER_HPP
