/**
 *
 *  @file SnapshotWriter.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Snapshot writer
 *
 */

#ifndef VIX_KV_SNAPSHOT_SNAPSHOT_WRITER_HPP
#define VIX_KV_SNAPSHOT_SNAPSHOT_WRITER_HPP

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
   * @brief Result returned after writing a snapshot.
   */
  struct SnapshotWriteResult
  {
    /**
     * @brief Snapshot identifier.
     */
    std::uint64_t snapshot_id{0};

    /**
     * @brief Snapshot file path.
     */
    std::filesystem::path path{};

    /**
     * @brief Snapshot file size in bytes.
     */
    std::uint64_t size_bytes{0};

    /**
     * @brief Number of entries written.
     */
    std::uint64_t entry_count{0};

    /**
     * @brief Highest sequence included in the snapshot.
     */
    std::uint64_t last_sequence{0};

    /**
     * @brief Returns true if the result has a snapshot id.
     *
     * @return true when snapshot_id is greater than zero.
     */
    [[nodiscard]] bool has_snapshot_id() const noexcept
    {
      return snapshot_id > 0;
    }

    /**
     * @brief Returns true if the result has a path.
     *
     * @return true when path is not empty.
     */
    [[nodiscard]] bool has_path() const noexcept
    {
      return !path.empty();
    }

    /**
     * @brief Returns true if the result has bytes.
     *
     * @return true when size_bytes is greater than zero.
     */
    [[nodiscard]] bool has_size() const noexcept
    {
      return size_bytes > 0;
    }

    /**
     * @brief Returns true if the result is structurally valid.
     *
     * Empty snapshots are allowed, but the written file should have bytes.
     *
     * @return true when id, path, and size are valid.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return has_snapshot_id() &&
             has_path() &&
             has_size();
    }
  };

  /**
   * @brief Writes KV snapshots to disk.
   *
   * SnapshotWriter serializes a Snapshot into a stable binary file.
   *
   * It writes to a temporary file first, then renames it to the final path.
   * This prevents publishing a partially written snapshot as the final file.
   *
   * Snapshot binary format:
   *
   * @code
   * uint32 magic
   * uint8  format_version
   * uint64 snapshot_id
   * uint64 last_sequence
   * uint64 created_at_ms
   * uint64 entry_count
   *
   * repeated entries:
   *   uint8  deleted
   *   uint64 sequence
   *   uint64 timestamp_ms
   *   uint32 key_size
   *   uint64 value_size
   *   bytes  key
   *   bytes  value
   *
   * uint32 payload_checksum
   * @endcode
   */
  class SnapshotWriter
  {
  public:
    /**
     * @brief Byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Creates a writer from config.
     *
     * @param config KV configuration.
     */
    explicit SnapshotWriter(core::KvConfig config);

    /**
     * @brief Creates a writer from explicit snapshot path.
     *
     * @param path Snapshot file path.
     */
    explicit SnapshotWriter(std::filesystem::path path);

    /**
     * @brief Writes a snapshot.
     *
     * The final path is derived from the snapshot id when the writer was
     * created from KvConfig.
     *
     * @param snapshot Snapshot to write.
     * @return Snapshot write result or KvError.
     */
    [[nodiscard]] core::KvResult<SnapshotWriteResult>
    write(const Snapshot &snapshot);

    /**
     * @brief Writes a snapshot to a specific final path.
     *
     * @param snapshot Snapshot to write.
     * @param final_path Final snapshot path.
     * @return Snapshot write result or KvError.
     */
    [[nodiscard]] core::KvResult<SnapshotWriteResult>
    write_to(
        const Snapshot &snapshot,
        const std::filesystem::path &final_path);

    /**
     * @brief Encodes a snapshot into bytes.
     *
     * @param snapshot Snapshot to encode.
     * @return Encoded bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    encode(const Snapshot &snapshot);

    /**
     * @brief Validates a snapshot before writing.
     *
     * @param snapshot Snapshot.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate(const Snapshot &snapshot);

    /**
     * @brief Returns writer config.
     *
     * @return KV config.
     */
    [[nodiscard]] const core::KvConfig &config() const noexcept;

    /**
     * @brief Returns the configured path.
     *
     * @return Snapshot path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

  private:
    /**
     * @brief Appends the snapshot header to the byte buffer.
     *
     * @param out Destination bytes.
     * @param snapshot Snapshot.
     */
    static void append_header(
        Bytes &out,
        const Snapshot &snapshot);

    /**
     * @brief Appends snapshot entries to the byte buffer.
     *
     * @param out Destination bytes.
     * @param snapshot Snapshot.
     */
    static void append_entries(
        Bytes &out,
        const Snapshot &snapshot);

    /**
     * @brief Appends the payload checksum to the byte buffer.
     *
     * @param out Destination bytes.
     */
    static void append_checksum(Bytes &out);

    /**
     * @brief Ensures the parent directory exists.
     *
     * @param final_path Final snapshot path.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    ensure_parent_directory(const std::filesystem::path &final_path);

    /**
     * @brief Writes bytes using temp file then atomic rename.
     *
     * @param bytes Encoded snapshot bytes.
     * @param final_path Final snapshot path.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    write_atomic(
        const Bytes &bytes,
        const std::filesystem::path &final_path);

  private:
    core::KvConfig config_{};
    std::filesystem::path path_{};
  };

} // namespace vix::kv::snapshot

#endif // VIX_KV_SNAPSHOT_SNAPSHOT_WRITER_HPP
