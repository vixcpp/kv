/**
 *
 *  @file KvError.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Structured KV error
 *
 */

#ifndef VIX_KV_CORE_KV_ERROR_HPP
#define VIX_KV_CORE_KV_ERROR_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <vix/kv/core/KvErrorCode.hpp>

namespace vix::kv::core
{
  /**
   * @brief Structured error returned by Vix KV.
   *
   * KvError carries:
   * - a stable error code
   * - a human-readable message
   * - an optional path
   * - an optional detail string
   *
   * The code is intended for program logic.
   * The message and detail are intended for humans, logs, and diagnostics.
   */
  class KvError
  {
  public:
    /**
     * @brief Creates an empty success-like error.
     */
    KvError() = default;

    /**
     * @brief Creates an error with a code and message.
     *
     * @param code Error code.
     * @param message Human-readable message.
     */
    KvError(KvErrorCode code, std::string message)
        : code_(code),
          message_(std::move(message))
    {
    }

    /**
     * @brief Creates an error with a code, message, and path.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @param path Related filesystem path.
     */
    KvError(
        KvErrorCode code,
        std::string message,
        std::filesystem::path path)
        : code_(code),
          message_(std::move(message)),
          path_(std::move(path))
    {
    }

    /**
     * @brief Creates an error with all fields.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @param path Related filesystem path.
     * @param detail Additional diagnostic detail.
     */
    KvError(
        KvErrorCode code,
        std::string message,
        std::filesystem::path path,
        std::string detail)
        : code_(code),
          message_(std::move(message)),
          path_(std::move(path)),
          detail_(std::move(detail))
    {
    }

    /**
     * @brief Creates a structured error.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError make(
        KvErrorCode code,
        std::string message)
    {
      return KvError(code, std::move(message));
    }

    /**
     * @brief Creates a structured error with a path.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @param path Related filesystem path.
     * @return KvError.
     */
    [[nodiscard]] static KvError make(
        KvErrorCode code,
        std::string message,
        std::filesystem::path path)
    {
      return KvError(
          code,
          std::move(message),
          std::move(path));
    }

    /**
     * @brief Creates a structured error with a path and detail.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @param path Related filesystem path.
     * @param detail Additional diagnostic detail.
     * @return KvError.
     */
    [[nodiscard]] static KvError make(
        KvErrorCode code,
        std::string message,
        std::filesystem::path path,
        std::string detail)
    {
      return KvError(
          code,
          std::move(message),
          std::move(path),
          std::move(detail));
    }

    /**
     * @brief Creates an invalid argument error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError invalid_argument(std::string message)
    {
      return make(
          KvErrorCode::InvalidArgument,
          std::move(message));
    }

    /**
     * @brief Creates an invalid key error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError invalid_key(std::string message)
    {
      return make(
          KvErrorCode::InvalidKey,
          std::move(message));
    }

    /**
     * @brief Creates a not found error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError not_found(std::string message)
    {
      return make(
          KvErrorCode::NotFound,
          std::move(message));
    }

    /**
     * @brief Creates an I/O error.
     *
     * @param message Human-readable message.
     * @param path Related filesystem path.
     * @return KvError.
     */
    [[nodiscard]] static KvError io(
        std::string message,
        std::filesystem::path path)
    {
      return make(
          KvErrorCode::IoError,
          std::move(message),
          std::move(path));
    }

    /**
     * @brief Creates a corruption error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError corruption(std::string message)
    {
      return make(
          KvErrorCode::Corruption,
          std::move(message));
    }

    /**
     * @brief Creates a checksum mismatch error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError checksum_mismatch(std::string message)
    {
      return make(
          KvErrorCode::ChecksumMismatch,
          std::move(message));
    }

    /**
     * @brief Creates a WAL error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError wal(std::string message)
    {
      return make(
          KvErrorCode::WalError,
          std::move(message));
    }

    /**
     * @brief Creates a storage error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError storage(std::string message)
    {
      return make(
          KvErrorCode::StorageError,
          std::move(message));
    }

    /**
     * @brief Creates a configuration error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError config(std::string message)
    {
      return make(
          KvErrorCode::ConfigError,
          std::move(message));
    }

    /**
     * @brief Creates an internal error.
     *
     * @param message Human-readable message.
     * @return KvError.
     */
    [[nodiscard]] static KvError internal(std::string message)
    {
      return make(
          KvErrorCode::InternalError,
          std::move(message));
    }

    /**
     * @brief Returns the error code.
     *
     * @return Error code.
     */
    [[nodiscard]] KvErrorCode code() const noexcept
    {
      return code_;
    }

    /**
     * @brief Returns the stable error code string.
     *
     * @return Error code string.
     */
    [[nodiscard]] std::string_view code_string() const noexcept
    {
      return to_string(code_);
    }

    /**
     * @brief Returns the human-readable message.
     *
     * @return Error message.
     */
    [[nodiscard]] const std::string &message() const noexcept
    {
      return message_;
    }

    /**
     * @brief Returns the optional path.
     *
     * @return Related filesystem path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
      return path_;
    }

    /**
     * @brief Returns the optional detail string.
     *
     * @return Diagnostic detail.
     */
    [[nodiscard]] const std::string &detail() const noexcept
    {
      return detail_;
    }

    /**
     * @brief Returns true when this error has a non-empty message.
     *
     * @return true if message is not empty.
     */
    [[nodiscard]] bool has_message() const noexcept
    {
      return !message_.empty();
    }

    /**
     * @brief Returns true when this error has a non-empty path.
     *
     * @return true if path is not empty.
     */
    [[nodiscard]] bool has_path() const noexcept
    {
      return !path_.empty();
    }

    /**
     * @brief Returns true when this error has extra detail.
     *
     * @return true if detail is not empty.
     */
    [[nodiscard]] bool has_detail() const noexcept
    {
      return !detail_.empty();
    }

    /**
     * @brief Returns true when the error code is Ok.
     *
     * @return true for Ok.
     */
    [[nodiscard]] bool is_ok() const noexcept
    {
      return core::is_ok(code_);
    }

    /**
     * @brief Returns true when the error code is not Ok.
     *
     * @return true for error codes.
     */
    [[nodiscard]] bool is_error() const noexcept
    {
      return core::is_error(code_);
    }

    /**
     * @brief Builds a readable diagnostic string.
     *
     * @return Combined diagnostic string.
     */
    [[nodiscard]] std::string to_string() const
    {
      std::string out;
      out += std::string(code_string());

      if (!message_.empty())
      {
        out += ": ";
        out += message_;
      }

      if (!path_.empty())
      {
        out += " [path: ";
        out += path_.string();
        out += "]";
      }

      if (!detail_.empty())
      {
        out += " [detail: ";
        out += detail_;
        out += "]";
      }

      return out;
    }

  private:
    KvErrorCode code_{KvErrorCode::Ok};
    std::string message_{};
    std::filesystem::path path_{};
    std::string detail_{};
  };

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_ERROR_HPP
