/**
 *
 *  @file test_memtable.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  MemTable unit tests
 *
 */

#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/core/KvLimits.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/memtable/MemTableEntry.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace memtable = vix::kv::memtable;

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

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  bool test_default_memtable_is_empty()
  {
    const memtable::MemTable table;

    return expect_true(
               table.empty(),
               "default memtable should be empty") &&
           expect_true(
               table.raw_empty(),
               "default memtable raw storage should be empty") &&
           expect_eq<std::size_t>(
               table.size(),
               0,
               "default memtable live size should be 0") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               0,
               "default memtable raw size should be 0") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "default memtable tombstone count should be 0") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               0,
               "default memtable last sequence should be 0");
  }

  bool test_put_live_entry()
  {
    memtable::MemTable table;

    auto result = table.put(
        "v1|5:hello",
        bytes("world"),
        1,
        100);

    if (!expect_true(result.is_ok(), "put should succeed"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "put key should be readable"))
    {
      return false;
    }

    return expect_true(
               table.contains("v1|5:hello"),
               "contains should return true for live key") &&
           expect_true(
               table.contains_raw("v1|5:hello"),
               "contains_raw should return true for live key") &&
           expect_eq<std::string>(
               entry->key,
               "v1|5:hello",
               "stored key should match") &&
           expect_true(
               entry->value == bytes("world"),
               "stored value should match") &&
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
               table.size(),
               1,
               "live size should be 1 after put") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               1,
               "raw size should be 1 after put") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               1,
               "last sequence should be updated after put");
  }

  bool test_put_replaces_existing_with_newer_sequence()
  {
    memtable::MemTable table;

    auto first = table.put(
        "v1|5:hello",
        bytes("old"),
        1,
        100);

    auto second = table.put(
        "v1|5:hello",
        bytes("new"),
        2,
        200);

    if (!expect_true(
            first.is_ok() && second.is_ok(),
            "both puts should succeed"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "replaced key should be readable"))
    {
      return false;
    }

    return expect_true(
               entry->value == bytes("new"),
               "newer put should replace value") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               2,
               "newer put should replace sequence") &&
           expect_eq<std::size_t>(
               table.size(),
               1,
               "live size should remain 1 after replace") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               1,
               "raw size should remain 1 after replace") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               2,
               "last sequence should become newer sequence");
  }

  bool test_older_sequence_does_not_overwrite_newer_entry()
  {
    memtable::MemTable table;

    auto newer = table.put(
        "v1|5:hello",
        bytes("new"),
        10,
        1000);

    auto older = table.put(
        "v1|5:hello",
        bytes("old"),
        5,
        500);

    if (!expect_true(
            newer.is_ok() && older.is_ok(),
            "newer and older puts should both return ok"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "key should still exist"))
    {
      return false;
    }

    return expect_true(
               entry->value == bytes("new"),
               "older put should not overwrite newer value") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               10,
               "older put should not overwrite newer sequence") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               10,
               "last sequence should remain the newer sequence");
  }

  bool test_erase_creates_tombstone()
  {
    memtable::MemTable table;

    auto put = table.put(
        "v1|5:hello",
        bytes("world"),
        1,
        100);

    auto erased = table.erase(
        "v1|5:hello",
        2,
        200);

    if (!expect_true(
            put.is_ok() && erased.is_ok(),
            "put and erase should succeed"))
    {
      return false;
    }

    auto live = table.get("v1|5:hello");
    auto raw = table.get_raw("v1|5:hello");

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
               !table.contains("v1|5:hello"),
               "contains should be false after erase") &&
           expect_true(
               table.contains_raw("v1|5:hello"),
               "contains_raw should be true after erase") &&
           expect_true(
               raw->is_tombstone(),
               "raw entry should be tombstone") &&
           expect_eq<std::size_t>(
               table.size(),
               0,
               "live size should be 0 after erase") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               1,
               "raw size should keep tombstone") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               1,
               "tombstone count should be 1") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               2,
               "last sequence should be updated by erase");
  }

  bool test_older_erase_does_not_delete_newer_live_value()
  {
    memtable::MemTable table;

    auto put = table.put(
        "v1|5:hello",
        bytes("world"),
        10,
        1000);

    auto erased = table.erase(
        "v1|5:hello",
        5,
        500);

    if (!expect_true(
            put.is_ok() && erased.is_ok(),
            "put and older erase should both return ok"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(
            entry.has_value(),
            "older erase should not hide newer live value"))
    {
      return false;
    }

    return expect_true(
               entry->value == bytes("world"),
               "older erase should not change value") &&
           expect_eq<std::uint64_t>(
               entry->sequence,
               10,
               "older erase should not change sequence") &&
           expect_eq<std::size_t>(
               table.size(),
               1,
               "live size should remain 1") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "older erase should not create tombstone");
  }

  bool test_newer_put_replaces_tombstone()
  {
    memtable::MemTable table;

    auto erased = table.erase(
        "v1|5:hello",
        2,
        200);

    auto put = table.put(
        "v1|5:hello",
        bytes("world"),
        3,
        300);

    if (!expect_true(
            erased.is_ok() && put.is_ok(),
            "erase and newer put should succeed"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(
            entry.has_value(),
            "newer put should restore live entry after tombstone"))
    {
      return false;
    }

    return expect_true(
               entry->is_live(),
               "newer put should create live entry") &&
           expect_true(
               entry->value == bytes("world"),
               "newer put should store value") &&
           expect_eq<std::size_t>(
               table.size(),
               1,
               "live size should be 1 after restoring key") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "tombstone count should be 0 after restoring key") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               1,
               "raw size should remain 1 after replacing tombstone");
  }

  bool test_apply_live_entry()
  {
    memtable::MemTable table;

    auto result = table.apply(
        memtable::MemTableEntry::live(
            "v1|5:hello",
            bytes("world"),
            1,
            100));

    if (!expect_true(result.is_ok(), "apply live entry should succeed"))
    {
      return false;
    }

    auto entry = table.get("v1|5:hello");

    return expect_true(
               entry.has_value(),
               "applied live entry should be readable") &&
           expect_true(
               entry->value == bytes("world"),
               "applied live value should match") &&
           expect_eq<std::size_t>(
               table.size(),
               1,
               "live size should be 1 after apply");
  }

  bool test_apply_tombstone_entry()
  {
    memtable::MemTable table;

    auto result = table.apply(
        memtable::MemTableEntry::tombstone(
            "v1|5:hello",
            1,
            100));

    if (!expect_true(result.is_ok(), "apply tombstone should succeed"))
    {
      return false;
    }

    auto live = table.get("v1|5:hello");
    auto raw = table.get_raw("v1|5:hello");

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
               table.tombstone_count(),
               1,
               "tombstone count should be 1 after apply");
  }

  bool test_list_live_entries_by_prefix()
  {
    memtable::MemTable table;

    (void)table.put("v1|5:users1:1", bytes("Ada"), 1, 0);
    (void)table.put("v1|5:users1:2", bytes("Grace"), 2, 0);
    (void)table.put("v1|8:settings5:theme", bytes("dark"), 3, 0);

    const auto entries = table.list("v1|5:users");

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
    memtable::MemTable table;

    (void)table.put("v1|5:users1:1", bytes("Ada"), 1, 0);
    (void)table.put("v1|5:users1:2", bytes("Grace"), 2, 0);
    (void)table.erase("v1|5:users1:1", 3, 0);

    const auto entries = table.list("v1|5:users");

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
    memtable::MemTable table;

    (void)table.put("v1|5:users1:1", bytes("Ada"), 1, 0);
    (void)table.erase("v1|5:users1:1", 2, 0);

    const auto entries = table.list_raw("v1|5:users");

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
    memtable::MemTable table;

    (void)table.put("b", bytes("2"), 2, 0);
    (void)table.put("a", bytes("1"), 1, 0);

    const auto entries = table.list("");

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
    memtable::MemTable table;

    (void)table.put("a", bytes("1"), 1, 0);
    (void)table.erase("b", 2, 0);
    (void)table.erase("c", 3, 0);

    const auto removed = table.prune_tombstones();

    return expect_eq<std::size_t>(
               removed,
               2,
               "prune_tombstones should remove tombstones") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "tombstone count should be 0 after prune") &&
           expect_eq<std::size_t>(
               table.raw_size(),
               1,
               "raw size should keep only live entries") &&
           expect_true(
               table.contains("a"),
               "live entry should remain after prune");
  }

  bool test_remove_raw_live_entry()
  {
    memtable::MemTable table;

    (void)table.put("a", bytes("1"), 1, 0);

    const bool removed = table.remove_raw("a");

    return expect_true(
               removed,
               "remove_raw should remove existing live entry") &&
           expect_true(
               table.raw_empty(),
               "raw table should be empty after remove_raw") &&
           expect_eq<std::size_t>(
               table.size(),
               0,
               "live size should be 0 after remove_raw");
  }

  bool test_remove_raw_tombstone_entry()
  {
    memtable::MemTable table;

    (void)table.erase("a", 1, 0);

    const bool removed = table.remove_raw("a");

    return expect_true(
               removed,
               "remove_raw should remove existing tombstone") &&
           expect_true(
               table.raw_empty(),
               "raw table should be empty after removing tombstone") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "tombstone count should be 0 after removing tombstone");
  }

  bool test_remove_raw_missing_key_returns_false()
  {
    memtable::MemTable table;

    return expect_true(
        !table.remove_raw("missing"),
        "remove_raw should return false for missing key");
  }

  bool test_clear_resets_table()
  {
    memtable::MemTable table;

    (void)table.put("a", bytes("1"), 1, 0);
    (void)table.erase("b", 2, 0);

    table.clear();

    return expect_true(
               table.empty(),
               "clear should reset live state") &&
           expect_true(
               table.raw_empty(),
               "clear should reset raw storage") &&
           expect_eq<std::size_t>(
               table.tombstone_count(),
               0,
               "clear should reset tombstone count") &&
           expect_eq<std::uint64_t>(
               table.last_sequence(),
               0,
               "clear should reset last sequence");
  }

  bool test_empty_key_is_rejected()
  {
    memtable::MemTable table;

    auto result = table.put(
        "",
        bytes("value"),
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
        "empty memtable key should return InvalidKey");
  }

  bool test_zero_sequence_is_rejected()
  {
    memtable::MemTable table;

    auto result = table.put(
        "a",
        bytes("value"),
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

  bool test_large_value_is_rejected()
  {
    memtable::MemTable table;

    const std::vector<std::uint8_t> large_value(
        core::KvLimits::max_value_size + 1U,
        static_cast<std::uint8_t>('x'));

    auto result = table.put(
        "a",
        large_value,
        1,
        0);

    if (!expect_true(
            result.is_err(),
            "put with oversized value should be rejected"))
    {
      return false;
    }

    return expect_error_code(
        result.error().code(),
        core::KvErrorCode::InvalidArgument,
        "oversized value should return InvalidArgument");
  }

  bool test_byte_size()
  {
    memtable::MemTable table;

    (void)table.put("abc", bytes("de"), 1, 0);
    (void)table.erase("xyz", 2, 0);

    return expect_eq<std::uint64_t>(
        table.byte_size(),
        8,
        "byte_size should include key and value bytes for live and tombstone entries");
  }

  bool test_entries_access()
  {
    memtable::MemTable table;

    (void)table.put("a", bytes("1"), 1, 0);

    const auto &entries = table.entries();

    return expect_eq<std::size_t>(
               entries.size(),
               1,
               "entries should expose raw map") &&
           expect_true(
               entries.find("a") != entries.end(),
               "entries should contain inserted key");
  }
}

int main()
{
  if (!test_default_memtable_is_empty())
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

  if (!test_older_erase_does_not_delete_newer_live_value())
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

  if (!test_clear_resets_table())
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

  if (!test_large_value_is_rejected())
  {
    return 1;
  }

  if (!test_byte_size())
  {
    return 1;
  }

  if (!test_entries_access())
  {
    return 1;
  }

  std::cout << "kv_test_memtable passed\n";
  return 0;
}
