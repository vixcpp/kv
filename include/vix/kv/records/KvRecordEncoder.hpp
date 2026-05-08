/**
 *
 *  @file KvRecordEncoder.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KV record encoder
 *
 */

#ifndef VIX_KV_RECORDS_KV_RECORD_ENCODER_HPP
#define VIX_KV_RECORDS_KV_RECORD_ENCODER_HPP

#include <cstdint>
#include <span>
#include <vector>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/records/KvRecordHeader.hpp>

namespace vix::kv::records
{
  namespace core = vix::kv::core;

  /**
   * @brief Encodes KV records into stable binary bytes.
   *
   * Binary format:
   *
   * @code
   * header
   * key bytes
   * value bytes
   * @endcode
   *
   * Header format:
   *
   * @code
   * uint32 magic
   * uint8  format_version
   * uint8  record_type
   * uint16 header_size
   * uint64 sequence
   * uint64 timestamp_ms
   * uint32 key_size
   * uint64 value_size
   * uint32 header_checksum
   * uint32 payload_checksum
   * @endcode
   *
   * All integers are encoded little-endian.
   */
  class KvRecordEncoder
  {
  public:
    /**
     * @brief Encoded byte vector type.
     */
    using Bytes = std::vector<std::uint8_t>;

    /**
     * @brief Encodes a complete record.
     *
     * This computes and stores header and payload checksums.
     *
     * @param record Record to encode.
     * @return Encoded bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    encode(KvRecord record);

    /**
     * @brief Encodes only the record header.
     *
     * @param header Header to encode.
     * @return Encoded header bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    encode_header(const KvRecordHeader &header);

    /**
     * @brief Encodes a header with header_checksum forced to zero.
     *
     * Used for computing header checksum.
     *
     * @param header Header to encode.
     * @return Encoded header bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    encode_header_for_checksum(const KvRecordHeader &header);

    /**
     * @brief Encodes the record payload.
     *
     * Payload is key bytes followed by value bytes.
     *
     * @param record Record.
     * @return Payload bytes or KvError.
     */
    [[nodiscard]] static core::KvResult<Bytes>
    encode_payload(const KvRecord &record);

    /**
     * @brief Computes the header checksum.
     *
     * The checksum is computed over the encoded header with
     * header_checksum set to zero.
     *
     * @param header Header.
     * @return Header checksum or KvError.
     */
    [[nodiscard]] static core::KvResult<std::uint32_t>
    compute_header_checksum(const KvRecordHeader &header);

    /**
     * @brief Computes the payload checksum.
     *
     * The checksum is computed over key bytes followed by value bytes.
     *
     * @param record Record.
     * @return Payload checksum or KvError.
     */
    [[nodiscard]] static core::KvResult<std::uint32_t>
    compute_payload_checksum(const KvRecord &record);

    /**
     * @brief Validates a record before encoding.
     *
     * @param record Record.
     * @return Success or KvError.
     */
    [[nodiscard]] static core::KvResult<void>
    validate(const KvRecord &record);

  private:
    /**
     * @brief Encodes the header into an existing byte vector.
     *
     * @param out Destination bytes.
     * @param header Header.
     */
    static void append_header(
        Bytes &out,
        const KvRecordHeader &header);

    /**
     * @brief Appends record payload to an existing byte vector.
     *
     * @param out Destination bytes.
     * @param record Record.
     */
    static void append_payload(
        Bytes &out,
        const KvRecord &record);
  };

} // namespace vix::kv::records

#endif // VIX_KV_RECORDS_KV_RECORD_ENCODER_HPP
