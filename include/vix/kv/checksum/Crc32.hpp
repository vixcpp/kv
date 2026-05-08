/**
 *
 *  @file Crc32.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  CRC32 checksum helper
 *
 */

#ifndef VIX_KV_CHECKSUM_CRC32_HPP
#define VIX_KV_CHECKSUM_CRC32_HPP

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace vix::kv::checksum
{
  /**
   * @brief CRC32 checksum helper.
   *
   * Crc32 is used to detect corrupted records, WAL entries, segments,
   * and snapshots.
   *
   * The implementation uses the standard CRC32 polynomial:
   *
   * 0xEDB88320
   *
   * Rules:
   * - checksum of empty data is valid.
   * - checksum is deterministic across platforms.
   * - checksum is not cryptographic.
   * - checksum is for corruption detection, not security.
   */
  class Crc32
  {
  public:
    /**
     * @brief Initial CRC32 value.
     */
    static constexpr std::uint32_t initial_value = 0xFFFFFFFFU;

    /**
     * @brief Final XOR value.
     */
    static constexpr std::uint32_t final_xor_value = 0xFFFFFFFFU;

    /**
     * @brief CRC32 polynomial.
     */
    static constexpr std::uint32_t polynomial = 0xEDB88320U;

    /**
     * @brief Computes CRC32 for a byte span.
     *
     * @param data Input bytes.
     * @return CRC32 checksum.
     */
    [[nodiscard]] static std::uint32_t compute(
        std::span<const std::uint8_t> data) noexcept;

    /**
     * @brief Computes CRC32 for a byte vector.
     *
     * @param data Input bytes.
     * @return CRC32 checksum.
     */
    [[nodiscard]] static std::uint32_t compute(
        const std::vector<std::uint8_t> &data) noexcept;

    /**
     * @brief Computes CRC32 for a string view.
     *
     * The string bytes are processed as raw bytes.
     *
     * @param data Input bytes.
     * @return CRC32 checksum.
     */
    [[nodiscard]] static std::uint32_t compute(
        std::string_view data) noexcept;

    /**
     * @brief Updates an existing CRC32 state with more bytes.
     *
     * This method expects the caller to pass the current internal CRC state.
     *
     * For normal one-shot checksums, use compute().
     *
     * @param current Current CRC state.
     * @param data Bytes to add.
     * @return Updated CRC state.
     */
    [[nodiscard]] static std::uint32_t update(
        std::uint32_t current,
        std::span<const std::uint8_t> data) noexcept;

    /**
     * @brief Starts an incremental CRC32 calculation.
     *
     * @return Initial CRC state.
     */
    [[nodiscard]] static constexpr std::uint32_t begin() noexcept
    {
      return initial_value;
    }

    /**
     * @brief Finishes an incremental CRC32 calculation.
     *
     * @param current Current CRC state.
     * @return Final CRC32 checksum.
     */
    [[nodiscard]] static constexpr std::uint32_t finish(
        std::uint32_t current) noexcept
    {
      return current ^ final_xor_value;
    }

    /**
     * @brief Verifies that bytes match an expected checksum.
     *
     * @param data Input bytes.
     * @param expected Expected CRC32 checksum.
     * @return true when checksum matches.
     */
    [[nodiscard]] static bool verify(
        std::span<const std::uint8_t> data,
        std::uint32_t expected) noexcept;

    /**
     * @brief Verifies that bytes match an expected checksum.
     *
     * @param data Input bytes.
     * @param expected Expected CRC32 checksum.
     * @return true when checksum matches.
     */
    [[nodiscard]] static bool verify(
        const std::vector<std::uint8_t> &data,
        std::uint32_t expected) noexcept;

    /**
     * @brief Verifies that string bytes match an expected checksum.
     *
     * @param data Input bytes.
     * @param expected Expected CRC32 checksum.
     * @return true when checksum matches.
     */
    [[nodiscard]] static bool verify(
        std::string_view data,
        std::uint32_t expected) noexcept;

  private:
    /**
     * @brief Updates CRC state with one byte.
     *
     * @param current Current CRC state.
     * @param byte Byte to add.
     * @return Updated CRC state.
     */
    [[nodiscard]] static std::uint32_t update_byte(
        std::uint32_t current,
        std::uint8_t byte) noexcept;
  };

} // namespace vix::kv::checksum

#endif // VIX_KV_CHECKSUM_CRC32_HPP
