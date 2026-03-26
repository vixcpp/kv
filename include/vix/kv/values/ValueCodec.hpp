/**
 *
 *  @file ValueCodec.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 */
#ifndef VIX_KV_VALUES_VALUE_CODEC_HPP
#define VIX_KV_VALUES_VALUE_CODEC_HPP

#include <softadastra/store/types/Value.hpp>
#include <vix/kv/values/KvValue.hpp>

namespace vix::kv::values
{
  /**
   * @brief Encode/decode between KvValue and Softadastra Store Value.
   *
   * KvValue is the developer-facing abstraction.
   * softadastra::store::types::Value is the internal storage format.
   */
  class ValueCodec
  {
  public:
    /**
     * @brief Encode KvValue → Store Value
     */
    [[nodiscard]] static softadastra::store::types::Value
    encode(const KvValue &value)
    {
      softadastra::store::types::Value out;
      out.data = value.bytes();
      return out;
    }

    /**
     * @brief Decode Store Value → KvValue
     */
    [[nodiscard]] static KvValue
    decode(const softadastra::store::types::Value &value)
    {
      return KvValue::from_bytes(value.data);
    }
  };

} // namespace vix::kv::values

#endif
