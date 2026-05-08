/**
 *
 *  @file test_key_encoder.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KeyEncoder unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/keys/KeyEncoder.hpp>
#include <vix/kv/keys/KeyPath.hpp>

#include <iostream>
#include <string>

namespace
{
  namespace core = vix::kv::core;
  namespace keys = vix::kv::keys;

  bool expect_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      return false;
    }

    return true;
  }

  template <typename T>
  bool expect_eq(
      const T &actual,
      const T &expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: " << expected << '\n';
      std::cerr << "  actual  : " << actual << '\n';
      return false;
    }

    return true;
  }

  bool expect_error_code(
      core::KvErrorCode actual,
      core::KvErrorCode expected,
      const char *message)
  {
    if (actual != expected)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::cerr << "  expected: "
                << core::to_string(expected)
                << '\n';
      std::cerr << "  actual  : "
                << core::to_string(actual)
                << '\n';
      return false;
    }

    return true;
  }

  bool test_encode_single_segment()
  {
    const keys::KeyPath path{"hello"};

    auto encoded = keys::KeyEncoder::encode(path);

    if (!expect_true(encoded.is_ok(), "single segment encode should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        encoded.value(),
        "v1|5:hello",
        "single segment should use stable length-prefixed format");
  }

  bool test_encode_multi_segment()
  {
    const keys::KeyPath path{"users", "42", "profile"};

    auto encoded = keys::KeyEncoder::encode(path);

    if (!expect_true(encoded.is_ok(), "multi-segment encode should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        encoded.value(),
        "v1|5:users2:427:profile",
        "multi-segment key should use stable length-prefixed format");
  }

  bool test_encode_to_string_alias()
  {
    const keys::KeyPath path{"settings", "theme"};

    auto encoded = keys::KeyEncoder::encode_to_string(path);

    if (!expect_true(encoded.is_ok(), "encode_to_string should succeed"))
    {
      return false;
    }

    return expect_eq<std::string>(
        encoded.value(),
        "v1|8:settings5:theme",
        "encode_to_string should match encode format");
  }

  bool test_encode_empty_path_is_rejected()
  {
    const keys::KeyPath path;

    auto encoded = keys::KeyEncoder::encode(path);

    if (!expect_true(encoded.is_err(), "empty key path should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        encoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty key path encode should return InvalidKey");
  }

  bool test_encode_empty_segment_is_rejected()
  {
    const keys::KeyPath path{"users", "", "name"};

    auto encoded = keys::KeyEncoder::encode(path);

    if (!expect_true(encoded.is_err(), "empty segment should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        encoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty segment encode should return InvalidKey");
  }

  bool test_encode_or_empty_success()
  {
    const keys::KeyPath path{"users", "1"};

    const auto encoded = keys::KeyEncoder::encode_or_empty(path);

    return expect_eq<std::string>(
        encoded,
        "v1|5:users1:1",
        "encode_or_empty should return encoded key for valid path");
  }

  bool test_encode_or_empty_failure()
  {
    const keys::KeyPath path;

    const auto encoded = keys::KeyEncoder::encode_or_empty(path);

    return expect_true(
        encoded.empty(),
        "encode_or_empty should return empty string for invalid path");
  }

  bool test_decode_single_segment()
  {
    auto decoded = keys::KeyEncoder::decode("v1|5:hello");

    if (!expect_true(decoded.is_ok(), "single segment decode should succeed"))
    {
      return false;
    }

    return expect_eq<std::size_t>(
               decoded.value().size(),
               1,
               "decoded single segment path should have one segment") &&
           expect_eq<std::string>(
               decoded.value().at(0),
               "hello",
               "decoded single segment should match");
  }

  bool test_decode_multi_segment()
  {
    auto decoded = keys::KeyEncoder::decode(
        "v1|5:users2:427:profile");

    if (!expect_true(decoded.is_ok(), "multi-segment decode should succeed"))
    {
      return false;
    }

    return expect_eq<std::size_t>(
               decoded.value().size(),
               3,
               "decoded multi-segment path should have 3 segments") &&
           expect_eq<std::string>(
               decoded.value().at(0),
               "users",
               "decoded first segment should match") &&
           expect_eq<std::string>(
               decoded.value().at(1),
               "42",
               "decoded second segment should match") &&
           expect_eq<std::string>(
               decoded.value().at(2),
               "profile",
               "decoded third segment should match");
  }

  bool test_encode_decode_roundtrip()
  {
    const keys::KeyPath original{"users", "42", "profile", "name"};

    auto encoded = keys::KeyEncoder::encode(original);

    if (!expect_true(encoded.is_ok(), "roundtrip encode should succeed"))
    {
      return false;
    }

    auto decoded = keys::KeyEncoder::decode(encoded.value());

    if (!expect_true(decoded.is_ok(), "roundtrip decode should succeed"))
    {
      return false;
    }

    return expect_true(
        decoded.value() == original,
        "decoded key path should equal original key path");
  }

  bool test_decode_empty_string_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("");

    if (!expect_true(decoded.is_err(), "empty encoded key should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty encoded key should return InvalidKey");
  }

  bool test_decode_unsupported_prefix_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("v2|5:hello");

    if (!expect_true(decoded.is_err(), "unsupported prefix should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "unsupported prefix should return InvalidKey");
  }

  bool test_decode_missing_size_separator_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("v1|5hello");

    if (!expect_true(
            decoded.is_err(),
            "encoded key without size separator should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "missing separator should return InvalidKey");
  }

  bool test_decode_zero_size_segment_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("v1|0:");

    if (!expect_true(
            decoded.is_err(),
            "zero-size encoded segment should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "zero-size segment should return InvalidKey");
  }

  bool test_decode_truncated_segment_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("v1|5:hel");

    if (!expect_true(
            decoded.is_err(),
            "truncated encoded segment should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "truncated segment should return InvalidKey");
  }

  bool test_decode_non_numeric_size_is_rejected()
  {
    auto decoded = keys::KeyEncoder::decode("v1|x:hello");

    if (!expect_true(
            decoded.is_err(),
            "non-numeric segment size should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        decoded.error().code(),
        core::KvErrorCode::InvalidKey,
        "non-numeric segment size should return InvalidKey");
  }

  bool test_prefix_collision_is_impossible()
  {
    const keys::KeyPath left{"ab", "c"};
    const keys::KeyPath right{"a", "bc"};

    auto encoded_left = keys::KeyEncoder::encode(left);
    auto encoded_right = keys::KeyEncoder::encode(right);

    if (!expect_true(
            encoded_left.is_ok() && encoded_right.is_ok(),
            "both collision test keys should encode successfully"))
    {
      return false;
    }

    return expect_true(
        encoded_left.value() != encoded_right.value(),
        "length-prefixed encoding should avoid segment-boundary collisions");
  }
}

int main()
{
  if (!test_encode_single_segment())
  {
    return 1;
  }

  if (!test_encode_multi_segment())
  {
    return 1;
  }

  if (!test_encode_to_string_alias())
  {
    return 1;
  }

  if (!test_encode_empty_path_is_rejected())
  {
    return 1;
  }

  if (!test_encode_empty_segment_is_rejected())
  {
    return 1;
  }

  if (!test_encode_or_empty_success())
  {
    return 1;
  }

  if (!test_encode_or_empty_failure())
  {
    return 1;
  }

  if (!test_decode_single_segment())
  {
    return 1;
  }

  if (!test_decode_multi_segment())
  {
    return 1;
  }

  if (!test_encode_decode_roundtrip())
  {
    return 1;
  }

  if (!test_decode_empty_string_is_rejected())
  {
    return 1;
  }

  if (!test_decode_unsupported_prefix_is_rejected())
  {
    return 1;
  }

  if (!test_decode_missing_size_separator_is_rejected())
  {
    return 1;
  }

  if (!test_decode_zero_size_segment_is_rejected())
  {
    return 1;
  }

  if (!test_decode_truncated_segment_is_rejected())
  {
    return 1;
  }

  if (!test_decode_non_numeric_size_is_rejected())
  {
    return 1;
  }

  if (!test_prefix_collision_is_impossible())
  {
    return 1;
  }

  std::cout << "kv_test_key_encoder passed\n";
  return 0;
}
