/**
 *
 *  @file KeyEncoder.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#include <vix/kv/keys/KeyEncoder.hpp>

namespace vix::kv::keys
{
  softadastra::store::types::Key
  KeyEncoder::encode(const KeyPath &path)
  {
    return softadastra::store::types::Key{
        encode_to_string(path)};
  }

  std::string
  KeyEncoder::encode_to_string(const KeyPath &path)
  {
    if (path.empty())
    {
      return {};
    }

    const auto &segments = path.segments();

    std::size_t total_size = 0;
    for (const auto &segment : segments)
    {
      total_size += segment.size();
    }

    total_size += (segments.size() - 1);

    std::string encoded;
    encoded.reserve(total_size);

    for (std::size_t i = 0; i < segments.size(); ++i)
    {
      if (i > 0)
      {
        encoded.push_back(separator());
      }

      encoded += segments[i];
    }

    return encoded;
  }

} // namespace vix::kv::keys
