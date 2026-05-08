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
 *  Public API result aliases
 *
 */

#ifndef VIX_KV_API_KV_RESULT_HPP
#define VIX_KV_API_KV_RESULT_HPP

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvResult.hpp>

namespace vix::kv::api
{
  namespace core = vix::kv::core;

  /**
   * @brief Public KV API result type.
   *
   * This alias exposes the core result type through the public API namespace.
   *
   * It is useful for developers who prefer importing only:
   *
   * @code
   * #include <vix/kv/api/KvResult.hpp>
   * @endcode
   *
   * instead of depending directly on internal core headers.
   *
   * Example:
   *
   * @code
   * vix::kv::api::KvResult<void> result = db.set({"user", "1"}, "Ada");
   *
   * if (result.is_err())
   * {
   *   std::cerr << result.error().message() << '\n';
   * }
   * @endcode
   */
  template <typename T>
  using KvResult = core::KvResult<T>;

  /**
   * @brief Public KV API error type.
   */
  using KvError = core::KvError;

  /**
   * @brief Public KV API error code type.
   */
  using KvErrorCode = core::KvErrorCode;

} // namespace vix::kv::api

#endif // VIX_KV_API_KV_RESULT_HPP
