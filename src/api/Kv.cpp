/**
 *
 *  @file Kv.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Public KV database API implementation
 *
 */

#include <vix/kv/api/Kv.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>

namespace vix::kv::api
{
  Kv::Kv()
      : Kv(KvOptions{})
  {
  }

  Kv::Kv(KvOptions options)
      : options_(std::move(options))
  {
    engine_ = std::make_unique<internal::KvEngine>(
        options_.to_config());
  }

  Kv::Kv(
      KvOptions options,
      internal::KvEngine engine)
      : options_(std::move(options)),
        engine_(std::make_unique<internal::KvEngine>(
            std::move(engine)))
  {
  }

  Kv::Kv(Kv &&other) noexcept
  {
    move_from(std::move(other));
  }

  Kv &Kv::operator=(Kv &&other) noexcept
  {
    if (this != &other)
    {
      if (engine_ && engine_->is_open())
      {
        (void)engine_->close();
      }

      move_from(std::move(other));
    }

    return *this;
  }

  Kv::~Kv()
  {
    if (engine_ && engine_->is_open())
    {
      (void)engine_->close();
    }
  }

  core::KvResult<Kv> Kv::open()
  {
    return open(KvOptions{});
  }

  core::KvResult<Kv> Kv::open(const KvOptions &options)
  {
    auto validation = options.validate();

    if (validation.is_err())
    {
      return core::KvResult<Kv>::err(validation.error());
    }

    Kv kv{options};

    auto opened = kv.open_handle();

    if (opened.is_err())
    {
      return core::KvResult<Kv>::err(opened.error());
    }

    return core::KvResult<Kv>::ok(std::move(kv));
  }

  core::KvResult<void> Kv::open_handle()
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return engine_result;
    }

    return engine_->open();
  }

  core::KvResult<void> Kv::close()
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return engine_result;
    }

    return engine_->close();
  }

  core::KvResult<void> Kv::flush()
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return engine_result;
    }

    return engine_->flush();
  }

  core::KvResult<void> Kv::set(
      const keys::KeyPath &key,
      const values::KvValue &value)
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return engine_result;
    }

    return engine_->set(key, value);
  }

  core::KvResult<void> Kv::set(
      const keys::KeyPath &key,
      std::string_view value)
  {
    return set(
        key,
        values::KvValue::from_string(value));
  }

  void Kv::put(
      std::string_view key,
      std::string_view value)
  {
    auto result = set(make_key_path(key), value);

    if (result.is_err())
    {
      throw std::runtime_error(result.error().to_string());
    }
  }

  std::optional<std::string> Kv::get(
      std::string_view key) const
  {
    auto result = get(make_key_path(key));

    if (result.is_err())
    {
      if (result.error().code() == core::KvErrorCode::NotFound)
      {
        return std::nullopt;
      }

      throw std::runtime_error(result.error().to_string());
    }

    return result.value().to_string();
  }

  core::KvResult<void> Kv::set(
      std::initializer_list<std::string_view> key,
      std::string_view value)
  {
    return set(
        make_key_path(key),
        value);
  }

  core::KvResult<values::KvValue> Kv::get(
      std::initializer_list<std::string_view> key) const
  {
    return get(make_key_path(key));
  }

  bool Kv::contains(
      std::initializer_list<std::string_view> key) const
  {
    return contains(make_key_path(key));
  }

  core::KvResult<Kv::ListResult> Kv::list(
      std::initializer_list<std::string_view> prefix) const
  {
    return list(make_key_path(prefix));
  }

  keys::KeyPath Kv::make_key_path(
      std::string_view key)
  {
    std::vector<std::string> parts;

    std::size_t start = 0;

    while (start <= key.size())
    {
      const std::size_t end = key.find('/', start);

      const std::size_t count =
          end == std::string_view::npos
              ? key.size() - start
              : end - start;

      if (count > 0)
      {
        parts.emplace_back(key.substr(start, count));
      }

      if (end == std::string_view::npos)
      {
        break;
      }

      start = end + 1;
    }

    return keys::KeyPath(std::move(parts));
  }

  keys::KeyPath Kv::make_key_path(
      std::initializer_list<std::string_view> parts)
  {
    std::vector<std::string> values;
    values.reserve(parts.size());

    for (std::string_view part : parts)
    {
      values.emplace_back(part);
    }

    return keys::KeyPath(std::move(values));
  }

  core::KvResult<values::KvValue> Kv::get(
      const keys::KeyPath &key) const
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return core::KvResult<values::KvValue>::err(
          engine_result.error());
    }

    return engine_->get(key);
  }

  core::KvResult<void> Kv::erase(
      const keys::KeyPath &key)
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return engine_result;
    }

    return engine_->erase(key);
  }

  bool Kv::contains(const keys::KeyPath &key) const
  {
    if (!engine_)
    {
      return false;
    }

    return engine_->contains(key);
  }

  core::KvResult<Kv::ListResult> Kv::list(
      const keys::KeyPath &prefix) const
  {
    auto engine_result = require_engine();

    if (engine_result.is_err())
    {
      return core::KvResult<ListResult>::err(
          engine_result.error());
    }

    return engine_->list(prefix);
  }

  std::size_t Kv::size() const noexcept
  {
    if (!engine_)
    {
      return 0;
    }

    return engine_->size();
  }

  bool Kv::empty() const noexcept
  {
    if (!engine_)
    {
      return true;
    }

    return engine_->empty();
  }

  bool Kv::is_open() const noexcept
  {
    return engine_ && engine_->is_open();
  }

  const KvOptions &Kv::options() const noexcept
  {
    return options_;
  }

  core::KvStats Kv::stats() const
  {
    if (!engine_)
    {
      return core::KvStats{};
    }

    return engine_->stats();
  }

  core::KvResult<void> Kv::require_engine() const
  {
    if (!engine_)
    {
      return core::KvResult<void>::err(
          core::KvError::make(
              core::KvErrorCode::NotOpen,
              "KV handle has no engine"));
    }

    return core::KvResult<void>::ok();
  }

  void Kv::move_from(Kv &&other) noexcept
  {
    options_ = std::move(other.options_);
    engine_ = std::move(other.engine_);
  }

} // namespace vix::kv::api
