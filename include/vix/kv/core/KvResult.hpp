/**
 *
 *  @file KvResult.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Result type for KV operations
 *
 */

#ifndef VIX_KV_CORE_KV_RESULT_HPP
#define VIX_KV_CORE_KV_RESULT_HPP

#include <type_traits>
#include <utility>
#include <variant>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvErrorCode.hpp>

namespace vix::kv::core
{
  /**
   * @brief Result type returned by Vix KV operations.
   *
   * KvResult<T> contains either:
   * - a success value of type T
   * - a KvError
   *
   * The caller must check is_ok() or is_err() before accessing value().
   *
   * Example:
   * @code
   * auto result = db.get({"users", "42"});
   *
   * if (result.is_err())
   * {
   *   std::cerr << result.error().message() << "\n";
   *   return;
   * }
   *
   * std::cout << result.value().to_string() << "\n";
   * @endcode
   */
  template <typename T>
  class KvResult
  {
  public:
    /**
     * @brief Success value type.
     */
    using ValueType = T;

    /**
     * @brief Error type.
     */
    using ErrorType = KvError;

    /**
     * @brief Creates a successful result from a value.
     *
     * @param value Success value.
     * @return Successful result.
     */
    [[nodiscard]] static KvResult ok(T value)
    {
      return KvResult(std::move(value));
    }

    /**
     * @brief Creates a failed result from an error.
     *
     * @param error Error value.
     * @return Failed result.
     */
    [[nodiscard]] static KvResult err(KvError error)
    {
      return KvResult(std::move(error));
    }

    /**
     * @brief Creates a failed result from an error code and message.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @return Failed result.
     */
    [[nodiscard]] static KvResult err(
        KvErrorCode code,
        std::string message)
    {
      return KvResult(
          KvError::make(code, std::move(message)));
    }

    /**
     * @brief Returns true when the result contains a value.
     *
     * @return true on success.
     */
    [[nodiscard]] bool is_ok() const noexcept
    {
      return std::holds_alternative<T>(data_);
    }

    /**
     * @brief Returns true when the result contains an error.
     *
     * @return true on failure.
     */
    [[nodiscard]] bool is_err() const noexcept
    {
      return std::holds_alternative<KvError>(data_);
    }

    /**
     * @brief Returns the success value.
     *
     * The caller must ensure is_ok() is true before calling this method.
     *
     * @return Success value.
     */
    [[nodiscard]] T &value()
    {
      return std::get<T>(data_);
    }

    /**
     * @brief Returns the success value.
     *
     * The caller must ensure is_ok() is true before calling this method.
     *
     * @return Success value.
     */
    [[nodiscard]] const T &value() const
    {
      return std::get<T>(data_);
    }

    /**
     * @brief Moves the success value out of the result.
     *
     * The caller must ensure is_ok() is true before calling this method.
     *
     * @return Moved success value.
     */
    [[nodiscard]] T &&move_value()
    {
      return std::move(std::get<T>(data_));
    }

    /**
     * @brief Returns the error value.
     *
     * The caller must ensure is_err() is true before calling this method.
     *
     * @return Error value.
     */
    [[nodiscard]] KvError &error()
    {
      return std::get<KvError>(data_);
    }

    /**
     * @brief Returns the error value.
     *
     * The caller must ensure is_err() is true before calling this method.
     *
     * @return Error value.
     */
    [[nodiscard]] const KvError &error() const
    {
      return std::get<KvError>(data_);
    }

    /**
     * @brief Returns the error code.
     *
     * Returns KvErrorCode::Ok when the result is successful.
     *
     * @return Error code.
     */
    [[nodiscard]] KvErrorCode code() const noexcept
    {
      if (is_ok())
      {
        return KvErrorCode::Ok;
      }

      return std::get<KvError>(data_).code();
    }

  private:
    /**
     * @brief Creates a successful result.
     *
     * @param value Success value.
     */
    explicit KvResult(T value)
        : data_(std::move(value))
    {
    }

    /**
     * @brief Creates a failed result.
     *
     * @param error Error value.
     */
    explicit KvResult(KvError error)
        : data_(std::move(error))
    {
    }

  private:
    std::variant<T, KvError> data_;
  };

  /**
   * @brief Specialization for operations that only return success or error.
   *
   * KvResult<void> contains either:
   * - success
   * - KvError
   */
  template <>
  class KvResult<void>
  {
  public:
    /**
     * @brief Error type.
     */
    using ErrorType = KvError;

    /**
     * @brief Creates a successful void result.
     *
     * @return Successful result.
     */
    [[nodiscard]] static KvResult ok()
    {
      return KvResult();
    }

    /**
     * @brief Creates a failed result from an error.
     *
     * @param error Error value.
     * @return Failed result.
     */
    [[nodiscard]] static KvResult err(KvError error)
    {
      KvResult result;
      result.error_ = std::move(error);
      result.ok_ = false;
      return result;
    }

    /**
     * @brief Creates a failed result from an error code and message.
     *
     * @param code Error code.
     * @param message Human-readable message.
     * @return Failed result.
     */
    [[nodiscard]] static KvResult err(
        KvErrorCode code,
        std::string message)
    {
      return err(
          KvError::make(code, std::move(message)));
    }

    /**
     * @brief Returns true when the operation succeeded.
     *
     * @return true on success.
     */
    [[nodiscard]] bool is_ok() const noexcept
    {
      return ok_;
    }

    /**
     * @brief Returns true when the operation failed.
     *
     * @return true on failure.
     */
    [[nodiscard]] bool is_err() const noexcept
    {
      return !ok_;
    }

    /**
     * @brief Returns the error value.
     *
     * The caller must ensure is_err() is true before calling this method.
     *
     * @return Error value.
     */
    [[nodiscard]] KvError &error() noexcept
    {
      return error_;
    }

    /**
     * @brief Returns the error value.
     *
     * The caller must ensure is_err() is true before calling this method.
     *
     * @return Error value.
     */
    [[nodiscard]] const KvError &error() const noexcept
    {
      return error_;
    }

    /**
     * @brief Returns the error code.
     *
     * Returns KvErrorCode::Ok when the result is successful.
     *
     * @return Error code.
     */
    [[nodiscard]] KvErrorCode code() const noexcept
    {
      if (ok_)
      {
        return KvErrorCode::Ok;
      }

      return error_.code();
    }

  private:
    /**
     * @brief Creates a successful result.
     */
    KvResult() = default;

  private:
    bool ok_{true};
    KvError error_{};
  };

} // namespace vix::kv::core

#endif // VIX_KV_CORE_KV_RESULT_HPP
