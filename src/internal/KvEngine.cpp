/**
 *
 *  @file KvEngine.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#include <vix/kv/internal/KvEngine.hpp>
#include <softadastra/store/core/Operation.hpp>
#include <softadastra/store/types/OperationType.hpp>

#include <sstream>

namespace vix::kv::internal
{
  namespace store_core = softadastra::store::core;
  namespace store_types = softadastra::store::types;

  KvEngine::KvEngine(store_core::StoreConfig config)
      : store_(std::move(config))
  {
  }

  void KvEngine::open()
  {
    store_.recover();
  }

  bool KvEngine::set(
      const keys::KeyPath &key,
      const values::KvValue &value)
  {
    store_core::Operation op;
    op.type = store_types::OperationType::Put;
    op.key = keys::KeyEncoder::encode(key);
    op.value = values::ValueCodec::encode(value);
    op.timestamp = 0;

    auto result = store_.apply_operation(op);
    return result.success;
  }

  std::optional<values::KvValue>
  KvEngine::get(const keys::KeyPath &key) const
  {
    auto store_key = keys::KeyEncoder::encode(key);

    auto entry = store_.get(store_key);
    if (!entry)
      return std::nullopt;

    return values::ValueCodec::decode(entry->value);
  }

  bool KvEngine::erase(const keys::KeyPath &key)
  {
    store_core::Operation op;
    op.type = store_types::OperationType::Delete;
    op.key = keys::KeyEncoder::encode(key);
    op.timestamp = 0;

    auto result = store_.apply_operation(op);
    return result.success;
  }

  std::vector<std::pair<keys::KeyPath, values::KvValue>>
  KvEngine::list(const keys::KeyPath &prefix) const
  {
    std::vector<std::pair<keys::KeyPath, values::KvValue>> result;

    const std::string encoded_prefix =
        keys::KeyEncoder::encode_to_string(prefix);

    const auto &entries = store_.entries();

    for (const auto &[key_str, entry] : entries)
    {
      if (!encoded_prefix.empty())
      {
        if (key_str.compare(0, encoded_prefix.size(), encoded_prefix) != 0)
          continue;
      }

      auto decoded_key = decode_key(key_str);
      auto decoded_value = values::ValueCodec::decode(entry.value);

      result.emplace_back(std::move(decoded_key), std::move(decoded_value));
    }

    return result;
  }

  keys::KeyPath
  KvEngine::decode_key(const std::string &encoded)
  {
    keys::KeyPath path;

    if (encoded.empty())
      return path;

    std::stringstream ss(encoded);
    std::string segment;

    while (std::getline(ss, segment, keys::KeyEncoder::separator()))
    {
      path.push_back(segment);
    }

    return path;
  }

} // namespace vix::kv::internal
