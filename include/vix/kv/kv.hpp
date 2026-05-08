/**
 *
 *  @file kv.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Main public include
 *
 */

#ifndef VIX_KV_KV_HPP
#define VIX_KV_KV_HPP

#include <vix/kv/api/Kv.hpp>
#include <vix/kv/api/KvOptions.hpp>
#include <vix/kv/api/KvResult.hpp>
#include <vix/kv/api/Open.hpp>

#include <vix/kv/core/KvError.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvResult.hpp>
#include <vix/kv/core/KvStats.hpp>

#include <vix/kv/keys/KeyPath.hpp>

#include <vix/kv/values/KvValue.hpp>

namespace vix::kv
{
  /**
   * @brief Public KV database handle.
   */
  using Kv = api::Kv;

  /**
   * @brief Public KV options.
   */
  using KvOptions = api::KvOptions;

  /**
   * @brief Public KV value.
   */
  using KvValue = values::KvValue;

  /**
   * @brief Public KV key path.
   */
  using KeyPath = keys::KeyPath;

  /**
   * @brief Public KV error.
   */
  using KvError = core::KvError;

  /**
   * @brief Public KV error code.
   */
  using KvErrorCode = core::KvErrorCode;

  /**
   * @brief Public KV stats.
   */
  using KvStats = core::KvStats;

  /**
   * @brief Public KV result type.
   */
  template <typename T>
  using KvResult = core::KvResult<T>;

} // namespace vix::kv

#endif // VIX_KV_KV_HPP
