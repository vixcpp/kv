/**
 *
 *  @file Open.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public open helpers implementation
 *
 */

#include <vix/kv/api/Open.hpp>

#include <filesystem>
#include <utility>
#include <stdexcept>

namespace vix::kv
{
  core::KvResult<api::Kv> open()
  {
    return api::Kv::open();
  }

  core::KvResult<api::Kv> open(
      const api::KvOptions &options)
  {
    return api::Kv::open(options);
  }

  api::Kv open(std::filesystem::path path)
  {
    auto opened = open_durable(std::move(path));

    if (opened.is_err())
    {
      throw std::runtime_error(opened.error().to_string());
    }

    return opened.move_value();
  }

  core::KvResult<api::Kv> open_memory()
  {
    return api::Kv::open(
        api::KvOptions::memory_only());
  }

  core::KvResult<api::Kv> open_durable(
      std::filesystem::path path)
  {
    return api::Kv::open(
        api::KvOptions::durable(std::move(path)));
  }

  core::KvResult<api::Kv> open_fast(
      std::filesystem::path path)
  {
    return api::Kv::open(
        api::KvOptions::fast(std::move(path)));
  }

} // namespace vix::kv
