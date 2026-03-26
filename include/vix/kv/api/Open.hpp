/**
 *
 *  @file Open.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#ifndef VIX_KV_API_OPEN_HPP
#define VIX_KV_API_OPEN_HPP

#include <vix/kv/api/Kv.hpp>
#include <vix/kv/api/KvOptions.hpp>

namespace vix::kv
{
  namespace api = vix::kv::api;

  /**
   * @brief Open a KV database with default options.
   *
   * Equivalent to:
   * Kv::open(KvOptions{})
   */
  [[nodiscard]] inline api::Kv open()
  {
    return api::Kv::open(api::KvOptions{});
  }

  /**
   * @brief Open a KV database with custom options.
   */
  [[nodiscard]] inline api::Kv open(const api::KvOptions &options)
  {
    return api::Kv::open(options);
  }

} // namespace vix::kv

#endif
