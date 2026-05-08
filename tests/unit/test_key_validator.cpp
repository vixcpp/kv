/**
 *
 *  @file test_key_validator.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KeyValidator unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/keys/KeyValidator.hpp>

#include <cstddef>
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
      const core::KvErrorCode actual,
      const core::KvErrorCode expected,
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

  bool test_valid_single_segment_key()
  {
    const keys::KeyPath path{"hello"};

    const auto result = keys::KeyValidator::validate(path);

    return expect_true(
        result.is_ok(),
        "single segment key should be valid");
  }

  bool test_valid_multi_segment_key()
  {
    const keys::KeyPath path{"users", "42", "profile", "name"};

    const auto result = keys::KeyValidator::validate(path);

    return expect_true(
        result.is_ok(),
        "multi-segment key should be valid");
  }

  bool test_empty_key_is_rejected()
  {
    const keys::KeyPath path;

    const auto result = keys::KeyValidator::validate(path);

    if (!expect_true(
            result.is_err(),
            "empty key path should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty key path should return InvalidKey");
  }

  bool test_empty_segment_is_rejected()
  {
    const keys::KeyPath path{"users", "", "name"};

    const auto result = keys::KeyValidator::validate(path);

    if (!expect_true(
            result.is_err(),
            "key path with empty segment should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty segment should return InvalidKey");
  }

  bool test_too_many_segments_is_rejected()
  {
    keys::KeyPath path;

    for (std::size_t index = 0;
         index < core::KvLimits::max_key_segments + 1U;
         ++index)
    {
      path.push_back("x");
    }

    const auto result = keys::KeyValidator::validate(path);

    if (!expect_true(
            result.is_err(),
            "key path with too many segments should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "too many segments should return InvalidKey");
  }

  bool test_segment_too_large_is_rejected()
  {
    const std::string large_segment(
        core::KvLimits::max_key_segment_size + 1U,
        'a');

    keys::KeyPath path;
    path.push_back(large_segment);

    const auto result = keys::KeyValidator::validate(path);

    if (!expect_true(
            result.is_err(),
            "key path with oversized segment should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "oversized segment should return InvalidKey");
  }

  bool test_total_key_size_too_large_is_rejected()
  {
    keys::KeyPath path;

    path.push_back(
        std::string(core::KvLimits::max_key_size, 'a'));

    path.push_back("b");

    const auto result = keys::KeyValidator::validate(path);

    if (!expect_true(
            result.is_err(),
            "key path with oversized total size should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "oversized total key size should return InvalidKey");
  }

  bool test_custom_limits_accept_valid_key()
  {
    const keys::KeyPath path{"users", "1"};

    const auto result = keys::KeyValidator::validate(
        path,
        2,
        16,
        32);

    return expect_true(
        result.is_ok(),
        "custom limits should accept valid key");
  }

  bool test_custom_max_segments_zero_is_rejected()
  {
    const keys::KeyPath path{"users"};

    const auto result = keys::KeyValidator::validate(
        path,
        0,
        16,
        32);

    if (!expect_true(
            result.is_err(),
            "custom max_segments 0 should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "custom max_segments 0 should return InvalidArgument");
  }

  bool test_custom_max_segment_size_zero_is_rejected()
  {
    const keys::KeyPath path{"users"};

    const auto result = keys::KeyValidator::validate(
        path,
        8,
        0,
        32);

    if (!expect_true(
            result.is_err(),
            "custom max_segment_size 0 should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "custom max_segment_size 0 should return InvalidArgument");
  }

  bool test_custom_max_total_size_zero_is_rejected()
  {
    const keys::KeyPath path{"users"};

    const auto result = keys::KeyValidator::validate(
        path,
        8,
        16,
        0);

    if (!expect_true(
            result.is_err(),
            "custom max_total_size 0 should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "custom max_total_size 0 should return InvalidArgument");
  }

  bool test_custom_segment_limit_is_enforced()
  {
    const keys::KeyPath path{"users", "1", "name"};

    const auto result = keys::KeyValidator::validate(
        path,
        2,
        16,
        64);

    if (!expect_true(
            result.is_err(),
            "custom segment limit should be enforced"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "custom segment limit failure should return InvalidKey");
  }

  bool test_custom_segment_size_limit_is_enforced()
  {
    const keys::KeyPath path{"users"};

    const auto result = keys::KeyValidator::validate(
        path,
        8,
        4,
        64);

    if (!expect_true(
            result.is_err(),
            "custom segment size limit should be enforced"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "custom segment size limit failure should return InvalidKey");
  }

  bool test_custom_total_size_limit_is_enforced()
  {
    const keys::KeyPath path{"users", "name"};

    const auto result = keys::KeyValidator::validate(
        path,
        8,
        16,
        8);

    if (!expect_true(
            result.is_err(),
            "custom total size limit should be enforced"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "custom total size limit failure should return InvalidKey");
  }

  bool test_boundary_segment_size_is_valid()
  {
    const std::string segment(
        core::KvLimits::max_key_segment_size,
        'a');

    keys::KeyPath path;
    path.push_back(segment);

    const auto result = keys::KeyValidator::validate(path);

    return expect_true(
        result.is_ok(),
        "segment exactly at max_key_segment_size should be valid");
  }

  bool test_boundary_total_size_is_valid()
  {
    keys::KeyPath path;

    std::size_t remaining = core::KvLimits::max_key_size;
    std::size_t segments_used = 0;

    while (remaining > 0)
    {
      if (segments_used >= core::KvLimits::max_key_segments)
      {
        return expect_true(
            false,
            "KvLimits are inconsistent: max_key_size cannot fit within max_key_segments * max_key_segment_size");
      }

      const std::size_t chunk_size =
          remaining > core::KvLimits::max_key_segment_size
              ? core::KvLimits::max_key_segment_size
              : remaining;

      path.push_back(std::string(chunk_size, 'a'));

      remaining -= chunk_size;
      ++segments_used;
    }

    const auto result = keys::KeyValidator::validate(path);

    return expect_true(
        result.is_ok(),
        "key exactly at max_key_size should be valid");
  }
}

int main()
{
  if (!test_valid_single_segment_key())
  {
    return 1;
  }

  if (!test_valid_multi_segment_key())
  {
    return 1;
  }

  if (!test_empty_key_is_rejected())
  {
    return 1;
  }

  if (!test_empty_segment_is_rejected())
  {
    return 1;
  }

  if (!test_too_many_segments_is_rejected())
  {
    return 1;
  }

  if (!test_segment_too_large_is_rejected())
  {
    return 1;
  }

  if (!test_total_key_size_too_large_is_rejected())
  {
    return 1;
  }

  if (!test_custom_limits_accept_valid_key())
  {
    return 1;
  }

  if (!test_custom_max_segments_zero_is_rejected())
  {
    return 1;
  }

  if (!test_custom_max_segment_size_zero_is_rejected())
  {
    return 1;
  }

  if (!test_custom_max_total_size_zero_is_rejected())
  {
    return 1;
  }

  if (!test_custom_segment_limit_is_enforced())
  {
    return 1;
  }

  if (!test_custom_segment_size_limit_is_enforced())
  {
    return 1;
  }

  if (!test_custom_total_size_limit_is_enforced())
  {
    return 1;
  }

  if (!test_boundary_segment_size_is_valid())
  {
    return 1;
  }

  if (!test_boundary_total_size_is_valid())
  {
    return 1;
  }

  std::cout << "kv_test_key_validator passed\n";
  return 0;
}
