/**
 *
 *  @file AtomicFile.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Atomic file write helper
 *
 */

#ifndef VIX_KV_UTILS_ATOMIC_FILE_HPP
#define VIX_KV_UTILS_ATOMIC_FILE_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>

namespace vix::kv::utils
{
  namespace core = vix::kv::core;

  /**
   * @brief Utility for writing files through temp file + rename.
   *
   * AtomicFile helps avoid publishing partially written files.
   *
   * Write flow:
   * - write bytes to <target>.tmp
   * - flush and close temp file
   * - remove old target if present
   * - rename temp file to target
   *
   * This is used by:
   * - snapshots
   * - manifests
   * - CURRENT file
   * - future metadata files
   */
  class AtomicFile
  {
  public:
    /**
     * @brief Byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Writes bytes atomically.
     *
     * @param target_path Final file path.
     * @param bytes Bytes to write.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void> write(
        const std::filesystem::path &target_path,
        const Bytes &bytes);

    /**
     * @brief Writes string bytes atomically.
     *
     * @param target_path Final file path.
     * @param text Text to write.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void> write_string(
        const std::filesystem::path &target_path,
        std::string_view text);

    /**
     * @brief Reads an entire file into bytes.
     *
     * @param path File path.
     * @return File bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes> read(
        const std::filesystem::path &path);

    /**
     * @brief Reads an entire file into a string.
     *
     * @param path File path.
     * @return File content or KvError.
     */
    [[nodiscard]] static core::KvResult<std::string> read_string(
        const std::filesystem::path &path);

    /**
     * @brief Builds the temporary path for a target path.
     *
     * @param target_path Final path.
     * @return Temporary path.
     */
    [[nodiscard]] static std::filesystem::path temporary_path(
        const std::filesystem::path &target_path);

    /**
     * @brief Ensures the parent directory exists.
     *
     * @param target_path File path.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    ensure_parent_directory(const std::filesystem::path &target_path);

  private:
    /**
     * @brief Writes bytes to a file path.
     *
     * @param path File path.
     * @param bytes Bytes to write.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void> write_direct(
        const std::filesystem::path &path,
        const Bytes &bytes);
  };

} // namespace vix::kv::utils

#endif // VIX_KV_UTILS_ATOMIC_FILE_HPP
