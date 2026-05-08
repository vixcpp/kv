/**
 *
 *  @file KvErrorCode.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Stable KV error codes
 *
 */

#ifndef VIX_KV_CORE_KV_ERROR_CODE_HPP
#define VIX_KV_CORE_KV_ERROR_CODE_HPP

#include <string_view>

namespace vix::kv::core
{
  /**
   * @brief Stable error codes returned by Vix KV.
   *
   * KvErrorCode is intended for program logic.
   *
   * Rules:
   * - Do not reorder existing values after release.
   * - Do not remove existing values after release.
   * - Add new values only at the end.
   * - Error messages may evolve, but error codes should remain stable.
   */
  enum class KvErrorCode
  {
    /**
     * @brief Operation completed successfully.
     */
    Ok = 0,

    /**
     * @brief Invalid argument passed by the caller.
     */
    InvalidArgument,

    /**
     * @brief Invalid key or key path.
     */
    InvalidKey,

    /**
     * @brief Requested key or resource was not found.
     */
    NotFound,

    /**
     * @brief Resource already exists.
     */
    AlreadyExists,

    /**
     * @brief KV engine is already open.
     */
    AlreadyOpen,

    /**
     * @brief KV engine is not open.
     */
    NotOpen,

    /**
     * @brief Filesystem or low-level I/O error.
     */
    IoError,

    /**
     * @brief Data corruption was detected.
     */
    Corruption,

    /**
     * @brief Checksum verification failed.
     */
    ChecksumMismatch,

    /**
     * @brief WAL operation failed.
     */
    WalError,

    /**
     * @brief Storage file or segment operation failed.
     */
    StorageError,

    /**
     * @brief Snapshot operation failed.
     */
    SnapshotError,

    /**
     * @brief Compaction operation failed.
     */
    CompactionError,

    /**
     * @brief Configuration is invalid.
     */
    ConfigError,

    /**
     * @brief Operation is not supported by this build or mode.
     */
    Unsupported,

    /**
     * @brief Internal invariant failed.
     */
    InternalError,

    /**
     * @brief Unknown error.
     */
    Unknown
  };

  /**
   * @brief Converts a KV error code to a stable string.
   *
   * @param code Error code.
   * @return Stable string representation.
   */
  [[nodiscard]] constexpr std::string_view
  to_string(KvErrorCode code) noexcept
  {
    switch (code)
    {
    case KvErrorCode::Ok:
      return "ok";

    case KvErrorCode::InvalidArgument:
      return "invalid_argument";

    case KvErrorCode::InvalidKey:
      return "invalid_key";

    case KvErrorCode::NotFound:
      return "not_found";

    case KvErrorCode::AlreadyExists:
      return "already_exists";

    case KvErrorCode::AlreadyOpen:
      return "already_open";

    case KvErrorCode::NotOpen:
      return "not_open";

    case KvErrorCode::IoError:
      return "io_error";

    case KvErrorCode::Corruption:
      return "corruption";

    case KvErrorCode::ChecksumMismatch:
      return "checksum_mismatch";

    case KvErrorCode::WalError:
      return "wal_error";

    case KvErrorCode::StorageError:
      return "storage_error";

    case KvErrorCode::SnapshotError:
      return "snapshot_error";

    case KvErrorCode::CompactionError:
      return "compaction_error";

    case KvErrorCode::ConfigError:
      return "config_error";

    case KvErrorCode::Unsupported:
      return "unsupported";

    case KvErrorCode::InternalError:
      return "internal_error";

    case KvErrorCode::Unknown:
      return "unknown";

    default:
      return "invalid_error_code";
    }
  }

  /**
   * @brief Returns true when the code represents success.
   *
   * @param code Error code.
   * @return true for Ok.
   */
  [[nodiscard]] constexpr bool
  is_ok(KvErrorCode code) noexcept
  {
    return code == KvErrorCode::Ok;
  }

  /**
   * @brief Returns true when the code represents failure.
   *
   * @param code Error code.
   * @return true for every code except Ok.
   */
  [[nodiscard]] constexpr bool
  is_error(KvErrorCode code) noexcept
  {
    return !is_ok(code);
  }

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_ERROR_CODE_HPP
