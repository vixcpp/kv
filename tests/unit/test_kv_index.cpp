/**
 *
 *  @file test_kv_index.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KvIndex unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/index/KvIndex.hpp>
#include <vix/kv/index/KvIndexEntry.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace
{
  namespace core = vix::kv::core;
  namespace index = vix::kv::index;

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

  bool test_default_index_is_empty()
  {
    const index::KvIndex idx;

    return expect_true(
               idx.empty(),
               "default index should be empty") &&
           expect_true(
               idx.raw_empty(),
               "default index raw storage should be empty") &&
           expect_eq<std::size_t>(
               idx.size(),
               0,
               "default index live size should be 0") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               0,
               "default index raw size should be 0") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "default index tombstone count should be 0") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               0,
               "default index last sequence should be 0");
  }

  bool test_put_live_entry()
  {
    index::KvIndex idx;

    auto result = idx.put(
        "v1|5:hello",
        1,
        10,
        44,
        1,
        100);

    if (!expect_true(result.is_ok(), "put should succeed"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "put key should be readable"))
    {
      return false;
    }

    return expect_true(
               idx.contains("v1|5:hello"),
               "contains should return true for live key") &&
           expect_true(
               idx.contains_raw("v1|5:hello"),
               "contains_raw should return true for live key") &&
           expect_eq<std::string>(
               entry->key,
               "v1|5:hello",
               "stored key should match") &&
           expect_eq<std::uint64_t>(
               entry->segment_id,
               1,
               "stored segment id should match") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               10,
               "stored offset should match") &&
           expect_eq<std::uint64_t>(
               entry->size,
               44,
               "stored record size should match") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               1,
               "stored sequence should match") &&
           expect_eq<std::uint64_t>(
               entry->timestamp_ms,
               100,
               "stored timestamp should match") &&
           expect_true(
               entry->is_live(),
               "stored entry should be live") &&
           expect_eq<std::size_t>(
               idx.size(),
               1,
               "live size should be 1 after put") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               1,
               "raw size should be 1 after put") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               1,
               "last sequence should be updated after put");
  }

  bool test_put_replaces_existing_with_newer_sequence()
  {
    index::KvIndex idx;

    auto first = idx.put(
        "v1|5:hello",
        1,
        10,
        44,
        1,
        100);

    auto second = idx.put(
        "v1|5:hello",
        2,
        99,
        55,
        2,
        200);

    if (!expect_true(
            first.is_ok() && second.is_ok(),
            "both puts should succeed"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "replaced key should be readable"))
    {
      return false;
    }

    return expect_eq<std::uint64_t>(
               entry->segment_id,
               2,
               "newer put should replace segment id") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               99,
               "newer put should replace offset") &&
           expect_eq<std::uint64_t>(
               entry->size,
               55,
               "newer put should replace size") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               2,
               "newer put should replace sequence") &&
           expect_eq<std::size_t>(
               idx.size(),
               1,
               "live size should remain 1 after replace") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               1,
               "raw size should remain 1 after replace") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               2,
               "last sequence should become newer sequence");
  }

  bool test_older_sequence_does_not_overwrite_newer_entry()
  {
    index::KvIndex idx;

    auto newer = idx.put(
        "v1|5:hello",
        10,
        100,
        44,
        10,
        1000);

    auto older = idx.put(
        "v1|5:hello",
        5,
        50,
        33,
        5,
        500);

    if (!expect_true(
            newer.is_ok() && older.is_ok(),
            "newer and older puts should both return ok"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "key should still exist"))
    {
      return false;
    }

    return expect_eq<std::uint64_t>(
               entry->segment_id,
               10,
               "older put should not overwrite segment id") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               100,
               "older put should not overwrite offset") &&
           expect_eq<std::uint64_t>(
               entry->size,
               44,
               "older put should not overwrite size") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               10,
               "older put should not overwrite sequence") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               10,
               "last sequence should remain the newer sequence");
  }

  bool test_erase_creates_tombstone()
  {
    index::KvIndex idx;

    auto put = idx.put(
        "v1|5:hello",
        1,
        10,
        44,
        1,
        100);

    auto erased = idx.erase(
        "v1|5:hello",
        1,
        54,
        22,
        2,
        200);

    if (!expect_true(
            put.is_ok() && erased.is_ok(),
            "put and erase should succeed"))
    {
      return false;
    }

    auto live = idx.get("v1|5:hello");
    auto raw = idx.get_raw("v1|5:hello");

    if (!expect_true(
            !live.has_value(),
            "get should hide tombstone entries"))
    {
      return false;
    }

    if (!expect_true(
            raw.has_value(),
            "get_raw should expose tombstone entries"))
    {
      return false;
    }

    return expect_true(
               !idx.contains("v1|5:hello"),
               "contains should be false after erase") &&
           expect_true(
               idx.contains_raw("v1|5:hello"),
               "contains_raw should be true after erase") &&
           expect_true(
               raw->is_tombstone(),
               "raw entry should be tombstone") &&
           expect_eq<std::uint64_t>(
               raw->segment_id,
               1,
               "tombstone segment id should match") &&
           expect_eq<std::uint64_t>(
               raw->offset,
               54,
               "tombstone offset should match") &&
           expect_eq<std::uint64_t>(
               raw->size,
               22,
               "tombstone size should match") &&
           expect_eq<std::size_t>(
               idx.size(),
               0,
               "live size should be 0 after erase") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               1,
               "raw size should keep tombstone") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               1,
               "tombstone count should be 1") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               2,
               "last sequence should be updated by erase");
  }

  bool test_older_erase_does_not_delete_newer_live_entry()
  {
    index::KvIndex idx;

    auto put = idx.put(
        "v1|5:hello",
        10,
        100,
        44,
        10,
        1000);

    auto erased = idx.erase(
        "v1|5:hello",
        5,
        50,
        22,
        5,
        500);

    if (!expect_true(
            put.is_ok() && erased.is_ok(),
            "put and older erase should both return ok"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    if (!expect_true(
            entry.has_value(),
            "older erase should not hide newer live entry"))
    {
      return false;
    }

    return expect_eq<std::uint64_t>(
               entry->segment_id,
               10,
               "older erase should not change segment id") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               100,
               "older erase should not change offset") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               10,
               "older erase should not change sequence") &&
           expect_eq<std::size_t>(
               idx.size(),
               1,
               "live size should remain 1") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "older erase should not create tombstone");
  }

  bool test_newer_put_replaces_tombstone()
  {
    index::KvIndex idx;

    auto erased = idx.erase(
        "v1|5:hello",
        1,
        10,
        22,
        2,
        200);

    auto put = idx.put(
        "v1|5:hello",
        3,
        99,
        44,
        3,
        300);

    if (!expect_true(
            erased.is_ok() && put.is_ok(),
            "erase and newer put should succeed"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    if (!expect_true(
            entry.has_value(),
            "newer put should restore live entry after tombstone"))
    {
      return false;
    }

    return expect_true(
               entry->is_live(),
               "newer put should create live entry") &&
           expect_eq<std::uint64_t>(
               entry->segment_id,
               3,
               "newer put should store new segment id") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               99,
               "newer put should store new offset") &&
           expect_eq<std::size_t>(
               idx.size(),
               1,
               "live size should be 1 after restoring key") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "tombstone count should be 0 after restoring key") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               1,
               "raw size should remain 1 after replacing tombstone");
  }

  bool test_apply_live_entry()
  {
    index::KvIndex idx;

    auto result = idx.apply(
        index::KvIndexEntry::live(
            "v1|5:hello",
            1,
            10,
            44,
            1,
            100));

    if (!expect_true(result.is_ok(), "apply live entry should succeed"))
    {
      return false;
    }

    auto entry = idx.get("v1|5:hello");

    return expect_true(
               entry.has_value(),
               "applied live entry should be readable") &&
           expect_eq<std::uint64_t>(
               entry->segment_id,
               1,
               "applied live segment id should match") &&
           expect_eq<std::uint64_t>(
               entry->offset,
               10,
               "applied live offset should match") &&
           expect_eq<std::size_t>(
               idx.size(),
               1,
               "live size should be 1 after apply");
  }

  bool test_apply_tombstone_entry()
  {
    index::KvIndex idx;

    auto result = idx.apply(
        index::KvIndexEntry::tombstone(
            "v1|5:hello",
            1,
            10,
            22,
            1,
            100));

    if (!expect_true(result.is_ok(), "apply tombstone should succeed"))
    {
      return false;
    }

    auto live = idx.get("v1|5:hello");
    auto raw = idx.get_raw("v1|5:hello");

    return expect_true(
               !live.has_value(),
               "applied tombstone should not be live") &&
           expect_true(
               raw.has_value(),
               "applied tombstone should be visible through raw access") &&
           expect_true(
               raw->is_tombstone(),
               "raw applied entry should be tombstone") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               1,
               "tombstone count should be 1 after apply");
  }

  bool test_list_live_entries_by_prefix()
  {
    index::KvIndex idx;

    (void)idx.put("v1|5:users1:1", 1, 10, 44, 1, 0);
    (void)idx.put("v1|5:users1:2", 1, 54, 44, 2, 0);
    (void)idx.put("v1|8:settings5:theme", 1, 98, 44, 3, 0);

    const auto entries = idx.list("v1|5:users");

    return expect_eq<std::size_t>(
               entries.size(),
               2,
               "list should return only live entries matching prefix") &&
           expect_eq<std::string>(
               entries[0].key,
               "v1|5:users1:1",
               "list should be sorted by key") &&
           expect_eq<std::string>(
               entries[1].key,
               "v1|5:users1:2",
               "list should be sorted by key");
  }

  bool test_list_skips_tombstones()
  {
    index::KvIndex idx;

    (void)idx.put("v1|5:users1:1", 1, 10, 44, 1, 0);
    (void)idx.put("v1|5:users1:2", 1, 54, 44, 2, 0);
    (void)idx.erase("v1|5:users1:1", 1, 98, 22, 3, 0);

    const auto entries = idx.list("v1|5:users");

    return expect_eq<std::size_t>(
               entries.size(),
               1,
               "list should skip tombstones") &&
           expect_eq<std::string>(
               entries[0].key,
               "v1|5:users1:2",
               "list should return remaining live key");
  }

  bool test_list_raw_includes_tombstones()
  {
    index::KvIndex idx;

    (void)idx.put("v1|5:users1:1", 1, 10, 44, 1, 0);
    (void)idx.erase("v1|5:users1:1", 1, 54, 22, 2, 0);

    const auto entries = idx.list_raw("v1|5:users");

    return expect_eq<std::size_t>(
               entries.size(),
               1,
               "list_raw should include tombstones") &&
           expect_true(
               entries[0].is_tombstone(),
               "list_raw entry should be tombstone");
  }

  bool test_empty_prefix_lists_all_live_entries()
  {
    index::KvIndex idx;

    (void)idx.put("b", 1, 20, 44, 2, 0);
    (void)idx.put("a", 1, 10, 44, 1, 0);

    const auto entries = idx.list("");

    return expect_eq<std::size_t>(
               entries.size(),
               2,
               "empty prefix should list all live entries") &&
           expect_eq<std::string>(
               entries[0].key,
               "a",
               "list should sort all entries") &&
           expect_eq<std::string>(
               entries[1].key,
               "b",
               "list should sort all entries");
  }

  bool test_prune_tombstones()
  {
    index::KvIndex idx;

    (void)idx.put("a", 1, 10, 44, 1, 0);
    (void)idx.erase("b", 1, 54, 22, 2, 0);
    (void)idx.erase("c", 1, 76, 22, 3, 0);

    const auto removed = idx.prune_tombstones();

    return expect_eq<std::size_t>(
               removed,
               2,
               "prune_tombstones should remove tombstones") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "tombstone count should be 0 after prune") &&
           expect_eq<std::size_t>(
               idx.raw_size(),
               1,
               "raw size should keep only live entries") &&
           expect_true(
               idx.contains("a"),
               "live entry should remain after prune");
  }

  bool test_remove_raw_live_entry()
  {
    index::KvIndex idx;

    (void)idx.put("a", 1, 10, 44, 1, 0);

    const bool removed = idx.remove_raw("a");

    return expect_true(
               removed,
               "remove_raw should remove existing live entry") &&
           expect_true(
               idx.raw_empty(),
               "raw index should be empty after remove_raw") &&
           expect_eq<std::size_t>(
               idx.size(),
               0,
               "live size should be 0 after remove_raw");
  }

  bool test_remove_raw_tombstone_entry()
  {
    index::KvIndex idx;

    (void)idx.erase("a", 1, 10, 22, 1, 0);

    const bool removed = idx.remove_raw("a");

    return expect_true(
               removed,
               "remove_raw should remove existing tombstone") &&
           expect_true(
               idx.raw_empty(),
               "raw index should be empty after removing tombstone") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "tombstone count should be 0 after removing tombstone");
  }

  bool test_remove_raw_missing_key_returns_false()
  {
    index::KvIndex idx;

    return expect_true(
        !idx.remove_raw("missing"),
        "remove_raw should return false for missing key");
  }

  bool test_clear_resets_index()
  {
    index::KvIndex idx;

    (void)idx.put("a", 1, 10, 44, 1, 0);
    (void)idx.erase("b", 1, 54, 22, 2, 0);

    idx.clear();

    return expect_true(
               idx.empty(),
               "clear should reset live state") &&
           expect_true(
               idx.raw_empty(),
               "clear should reset raw storage") &&
           expect_eq<std::size_t>(
               idx.tombstone_count(),
               0,
               "clear should reset tombstone count") &&
           expect_eq<std::uint64_t>(
               idx.last_sequence(),
               0,
               "clear should reset last sequence");
  }

  bool test_empty_key_is_rejected()
  {
    index::KvIndex idx;

    auto result = idx.put(
        "",
        1,
        10,
        44,
        1,
        0);

    if (!expect_true(
            result.is_err(),
            "put with empty key should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "empty index key should return InvalidKey");
  }

  bool test_zero_sequence_is_rejected()
  {
    index::KvIndex idx;

    auto result = idx.put(
        "a",
        1,
        10,
        44,
        0,
        0);

    if (!expect_true(
            result.is_err(),
            "put with sequence 0 should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "sequence 0 should return InvalidArgument");
  }

  bool test_zero_record_size_is_rejected()
  {
    index::KvIndex idx;

    auto result = idx.put(
        "a",
        1,
        10,
        0,
        1,
        0);

    if (!expect_true(
            result.is_err(),
            "put with record size 0 should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "record size 0 should return InvalidArgument");
  }

  bool test_large_key_is_rejected()
  {
    index::KvIndex idx;

    const std::string large_key(
        core::KvLimits::max_key_size + 1U,
        'x');

    auto result = idx.put(
        large_key,
        1,
        10,
        44,
        1,
        0);

    if (!expect_true(
            result.is_err(),
            "put with oversized key should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidKey,
        "oversized key should return InvalidKey");
  }

  bool test_large_record_size_is_rejected()
  {
    index::KvIndex idx;

    auto result = idx.put(
        "a",
        1,
        10,
        core::KvLimits::max_record_size + 1U,
        1,
        0);

    if (!expect_true(
            result.is_err(),
            "put with oversized record size should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "oversized record size should return InvalidArgument");
  }

  bool test_entries_access()
  {
    index::KvIndex idx;

    (void)idx.put("a", 1, 10, 44, 1, 0);

    const auto &entries = idx.entries();

    return expect_eq<std::size_t>(
               entries.size(),
               1,
               "entries should expose raw map") &&
           expect_true(
               entries.find("a") != entries.end(),
               "entries should contain inserted key");
  }

  bool test_index_entry_helpers()
  {
    auto live = index::KvIndexEntry::live(
        "a",
        1,
        10,
        44,
        1,
        100);

    auto tombstone = index::KvIndexEntry::tombstone(
        "b",
        2,
        20,
        22,
        2,
        200);

    const bool valid_initial_state =
        live.has_key() &&
        live.has_sequence() &&
        live.has_location() &&
        live.is_live() &&
        !live.is_tombstone() &&
        live.is_valid() &&
        tombstone.has_key() &&
        tombstone.has_sequence() &&
        tombstone.has_location() &&
        !tombstone.is_live() &&
        tombstone.is_tombstone() &&
        tombstone.is_valid();

    live.clear();

    return expect_true(
               valid_initial_state,
               "KvIndexEntry helpers should report correct state") &&
           expect_true(
               !live.has_key(),
               "clear should remove key") &&
           expect_true(
               !live.has_sequence(),
               "clear should reset sequence") &&
           expect_true(
               !live.has_location(),
               "clear should reset location") &&
           expect_true(
               live.is_live(),
               "clear should reset deleted flag to false") &&
           expect_true(
               !live.is_valid(),
               "cleared entry should be invalid");
  }
}

int main()
{
  if (!test_default_index_is_empty())
  {
    return 1;
  }

  if (!test_put_live_entry())
  {
    return 1;
  }

  if (!test_put_replaces_existing_with_newer_sequence())
  {
    return 1;
  }

  if (!test_older_sequence_does_not_overwrite_newer_entry())
  {
    return 1;
  }

  if (!test_erase_creates_tombstone())
  {
    return 1;
  }

  if (!test_older_erase_does_not_delete_newer_live_entry())
  {
    return 1;
  }

  if (!test_newer_put_replaces_tombstone())
  {
    return 1;
  }

  if (!test_apply_live_entry())
  {
    return 1;
  }

  if (!test_apply_tombstone_entry())
  {
    return 1;
  }

  if (!test_list_live_entries_by_prefix())
  {
    return 1;
  }

  if (!test_list_skips_tombstones())
  {
    return 1;
  }

  if (!test_list_raw_includes_tombstones())
  {
    return 1;
  }

  if (!test_empty_prefix_lists_all_live_entries())
  {
    return 1;
  }

  if (!test_prune_tombstones())
  {
    return 1;
  }

  if (!test_remove_raw_live_entry())
  {
    return 1;
  }

  if (!test_remove_raw_tombstone_entry())
  {
    return 1;
  }

  if (!test_remove_raw_missing_key_returns_false())
  {
    return 1;
  }

  if (!test_clear_resets_index())
  {
    return 1;
  }

  if (!test_empty_key_is_rejected())
  {
    return 1;
  }

  if (!test_zero_sequence_is_rejected())
  {
    return 1;
  }

  if (!test_zero_record_size_is_rejected())
  {
    return 1;
  }

  if (!test_large_key_is_rejected())
  {
    return 1;
  }

  if (!test_large_record_size_is_rejected())
  {
    return 1;
  }

  if (!test_entries_access())
  {
    return 1;
  }

  if (!test_index_entry_helpers())
  {
    return 1;
  }

  std::cout << "kv_test_kv_index passed\n";
  return 0;
}
