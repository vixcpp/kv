/**
 *
 *  @file KvLimits.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Central KV limits
 *
 */

#ifndef VIX_KV_CORE_KV_LIMITS_HPP
#define VIX_KV_CORE_KV_LIMITS_HPP

#include <cstddef>
#include <cstdint>

namespace vix::kv::core
{
  /**
   * @brief Central limits used by the Vix KV engine.
   *
   * KvLimits keeps size boundaries in one place so the encoder, decoder,
   * engine, WAL, storage, and public API can agree on safe defaults.
   */
  struct KvLimits
  {
    /**
     * @brief Maximum number of segments in a public KeyPath.
     */
    static constexpr std::size_t max_key_segments = 64;

    /**
     * @brief Maximum size of one key segment in bytes.
     */
    static constexpr std::size_t max_key_segment_size = 1024;

    /**
     * @brief Maximum encoded key size in bytes.
     *
     * Default: 1 MiB.
     */
    static constexpr std::size_t max_key_size =
        1024ULL * 1024ULL;

    /**
     * @brief Maximum value size in bytes.
     *
     * Default: 64 MiB.
     */
    static constexpr std::size_t max_value_size =
        64ULL * 1024ULL * 1024ULL;

    /**
     * @brief Maximum record payload size in bytes.
     *
     * This includes key bytes, value bytes, and record metadata.
     */
    static constexpr std::size_t max_record_size =
        max_key_size + max_value_size + 1024ULL;

    /**
     * @brief Default in-memory index capacity.
     */
    static constexpr std::size_t default_initial_capacity = 1024;

    /**
     * @brief Default maximum WAL file size before rotation or compaction.
     *
     * Default: 256 MiB.
     */
    static constexpr std::uint64_t default_max_wal_size =
        256ULL * 1024ULL * 1024ULL;

    /**
     * @brief Default maximum segment size.
     *
     * Default: 64 MiB.
     */
    static constexpr std::uint64_t default_segment_size =
        64ULL * 1024ULL * 1024ULL;

    /**
     * @brief Default maximum number of open files.
     */
    static constexpr std::size_t default_max_open_files = 64;

    /**
     * @brief Default maximum number of records processed per recovery batch.
     */
    static constexpr std::size_t default_recovery_batch_size = 4096;

    /**
     * @brief Current binary record format version.
     */
    static constexpr std::uint8_t record_format_version = 1;

    /**
     * @brief Current WAL format version.
     */
    static constexpr std::uint8_t wal_format_version = 1;

    /**
     * @brief Current snapshot format version.
     */
    static constexpr std::uint8_t snapshot_format_version = 1;

    /**
     * @brief Magic number used by KV records.
     *
     * ASCII-like marker: V K V R.
     */
    static constexpr std::uint32_t record_magic = 0x52564B56U;

    /**
     * @brief Magic number used by KV WAL files.
     *
     * ASCII-like marker: V K V W.
     */
    static constexpr std::uint32_t wal_magic = 0x57564B56U;

    /**
     * @brief Magic number used by KV snapshot files.
     *
     * ASCII-like marker: V K V S.
     */
    static constexpr std::uint32_t snapshot_magic = 0x53564B56U;

    /**
     * @brief Returns true if the key segment count is allowed.
     *
     * @param count Number of segments.
     * @return true when count is within limits.
     */
    [[nodiscard]] static constexpr bool
    valid_key_segment_count(std::size_t count) noexcept
    {
      return count > 0 && count <= max_key_segments;
    }

    /**
     * @brief Returns true if one key segment size is allowed.
     *
     * @param size Segment size in bytes.
     * @return true when size is within limits.
     */
    [[nodiscard]] static constexpr bool
    valid_key_segment_size(std::size_t size) noexcept
    {
      return size > 0 && size <= max_key_segment_size;
    }

    /**
     * @brief Returns true if encoded key size is allowed.
     *
     * @param size Key size in bytes.
     * @return true when size is within limits.
     */
    [[nodiscard]] static constexpr bool
    valid_key_size(std::size_t size) noexcept
    {
      return size > 0 && size <= max_key_size;
    }

    /**
     * @brief Returns true if value size is allowed.
     *
     * Empty values are allowed.
     *
     * @param size Value size in bytes.
     * @return true when size is within limits.
     */
    [[nodiscard]] static constexpr bool
    valid_value_size(std::size_t size) noexcept
    {
      return size <= max_value_size;
    }

    /**
     * @brief Returns true if record size is allowed.
     *
     * @param size Record size in bytes.
     * @return true when size is within limits.
     */
    [[nodiscard]] static constexpr bool
    valid_record_size(std::size_t size) noexcept
    {
      return size > 0 && size <= max_record_size;
    }

    /**
     * @brief Returns true if an initial capacity is usable.
     *
     * @param capacity Initial capacity.
     * @return true when capacity is greater than zero.
     */
    [[nodiscard]] static constexpr bool
    valid_initial_capacity(std::size_t capacity) noexcept
    {
      return capacity > 0;
    }

    /**
     * @brief Returns true if a segment size is usable.
     *
     * @param size Segment size in bytes.
     * @return true when size is greater than zero.
     */
    [[nodiscard]] static constexpr bool
    valid_segment_size(std::uint64_t size) noexcept
    {
      return size > 0;
    }

    /**
     * @brief Returns true if a WAL size limit is usable.
     *
     * @param size WAL size limit in bytes.
     * @return true when size is greater than zero.
     */
    [[nodiscard]] static constexpr bool
    valid_wal_size(std::uint64_t size) noexcept
    {
      return size > 0;
    }
  };

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_LIMITS_HPP
