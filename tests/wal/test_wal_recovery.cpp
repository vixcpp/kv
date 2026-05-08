/**
 *
 *  @file test_wal_recovery.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  WAL recovery unit tests
 *
 */

#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/core/KvErrorCode.hpp>
#include <vix/kv/memtable/MemTable.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/wal/KvWalRecovery.hpp>
#include <vix/kv/wal/KvWalWriter.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  namespace core = vix::kv::core;
  namespace memtable = vix::kv::memtable;
  namespace records = vix::kv::records;
  namespace wal = vix::kv::wal;

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

  std::filesystem::path make_test_root()
  {
    auto root =
        std::filesystem::temp_directory_path() /
        "vix_kv_test_wal_recovery";

    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    return root;
  }

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  records::KvRecord make_put_record(
      std::uint64_t sequence,
      std::string key,
      std::string value)
  {
    return records::KvRecord::put(
        std::move(key),
        bytes(std::move(value)),
        sequence,
        sequence * 100);
  }

  records::KvRecord make_delete_record(
      std::uint64_t sequence,
      std::string key)
  {
    return records::KvRecord::remove(
        std::move(key),
        sequence,
        sequence * 100);
  }

  records::KvRecord make_snapshot_record(
      std::uint64_t sequence)
  {
    return records::KvRecord::snapshot(
        "v1|8:snapshot",
        bytes("snapshot-payload"),
        sequence,
        sequence * 100);
  }

  records::KvRecord make_compaction_record(
      std::uint64_t sequence)
  {
    return records::KvRecord::compaction(
        "v1|10:compaction",
        sequence,
        sequence * 100);
  }

  bool write_records(
      const std::filesystem::path &wal_path,
      const std::vector<records::KvRecord> &items)
  {
    wal::KvWalWriter writer{wal_path};

    auto opened = writer.open();

    if (opened.is_err())
    {
      std::cerr << "failed to open writer: "
                << opened.error().message()
                << '\n';
      return false;
    }

    for (const auto &record : items)
    {
      auto appended = writer.append(record);

      if (appended.is_err())
      {
        std::cerr << "failed to append record: "
                  << appended.error().message()
                  << '\n';

        (void)writer.close();
        return false;
      }
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      std::cerr << "failed to close writer: "
                << closed.error().message()
                << '\n';
      return false;
    }

    return true;
  }

  bool test_recover_missing_wal_is_ok()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    const bool ok =
        expect_true(
            result.is_ok(),
            "recovering from a missing WAL should succeed") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            0,
            "missing WAL should recover 0 records") &&
        expect_eq<std::uint64_t>(
            result.value().bytes_read,
            0,
            "missing WAL should read 0 bytes") &&
        expect_eq<std::size_t>(
            table.size(),
            0,
            "missing WAL should leave memtable empty") &&
        expect_eq<std::size_t>(
            table.raw_size(),
            0,
            "missing WAL should leave raw memtable empty");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_single_put_record()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {make_put_record(1, "v1|5:hello", "world")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "recovered key should exist"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            entry->value == bytes("world"),
            "recovered value should match") &&
        expect_eq<std::uint64_t>(
            entry->sequence,
            1,
            "recovered sequence should match") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            1,
            "recovery should count one recovered record") &&
        expect_eq<std::uint64_t>(
            result.value().last_sequence,
            1,
            "recovery last_sequence should be 1") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            1,
            "recovery live_keys should be 1") &&
        expect_eq<std::uint64_t>(
            result.value().tombstones,
            0,
            "recovery tombstones should be 0") &&
        expect_true(
            result.value().bytes_read > 0,
            "recovery bytes_read should be greater than 0");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_multiple_put_records()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {
                make_put_record(1, "v1|5:users1:1", "Ada"),
                make_put_record(2, "v1|5:users1:2", "Grace"),
                make_put_record(3, "v1|8:settings5:theme", "dark"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            table.contains("v1|5:users1:1"),
            "first recovered key should exist") &&
        expect_true(
            table.contains("v1|5:users1:2"),
            "second recovered key should exist") &&
        expect_true(
            table.contains("v1|8:settings5:theme"),
            "third recovered key should exist") &&
        expect_eq<std::size_t>(
            table.size(),
            3,
            "memtable should contain 3 live keys") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            3,
            "recovery should count 3 recovered records") &&
        expect_eq<std::uint64_t>(
            result.value().last_sequence,
            3,
            "last recovered sequence should be 3") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            3,
            "recovery live_keys should be 3");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_delete_creates_tombstone()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {
                make_put_record(1, "v1|5:hello", "world"),
                make_delete_record(2, "v1|5:hello"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto live = table.get("v1|5:hello");
    auto raw = table.get_raw("v1|5:hello");

    if (!expect_true(
            !live.has_value(),
            "deleted key should not be live after recovery"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    if (!expect_true(
            raw.has_value(),
            "deleted key should exist as raw tombstone"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            raw->is_tombstone(),
            "raw recovered entry should be tombstone") &&
        expect_eq<std::uint64_t>(
            raw->sequence,
            2,
            "tombstone sequence should match delete record") &&
        expect_eq<std::size_t>(
            table.size(),
            0,
            "memtable live size should be 0 after recovered delete") &&
        expect_eq<std::size_t>(
            table.tombstone_count(),
            1,
            "memtable tombstone count should be 1") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            2,
            "recovery should count put and delete records") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            0,
            "recovery live_keys should be 0 after delete") &&
        expect_eq<std::uint64_t>(
            result.value().tombstones,
            1,
            "recovery tombstones should be 1");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_latest_sequence_wins()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {
                make_put_record(1, "v1|5:hello", "old"),
                make_put_record(2, "v1|5:hello", "new"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "recovered key should exist"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            entry->value == bytes("new"),
            "latest put should win during recovery") &&
        expect_eq<std::uint64_t>(
            entry->sequence,
            2,
            "latest sequence should win during recovery") &&
        expect_eq<std::uint64_t>(
            result.value().last_sequence,
            2,
            "last sequence should be 2");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_put_after_delete_restores_live_key()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {
                make_put_record(1, "v1|5:hello", "old"),
                make_delete_record(2, "v1|5:hello"),
                make_put_record(3, "v1|5:hello", "new"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(
            entry.has_value(),
            "put after delete should restore live key"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            entry->value == bytes("new"),
            "restored key should contain newest value") &&
        expect_eq<std::uint64_t>(
            entry->sequence,
            3,
            "restored key should use newest sequence") &&
        expect_eq<std::size_t>(
            table.size(),
            1,
            "memtable should have one live key") &&
        expect_eq<std::size_t>(
            table.tombstone_count(),
            0,
            "tombstone should be replaced by live key") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            3,
            "recovery should count all mutation records") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            1,
            "recovery live_keys should be 1");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_skips_snapshot_and_compaction_records()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {
                make_snapshot_record(1),
                make_compaction_record(2),
                make_put_record(3, "v1|5:hello", "world"),
            }))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            table.contains("v1|5:hello"),
            "put record should still be applied") &&
        expect_eq<std::uint64_t>(
            result.value().records_recovered,
            3,
            "recovery should count skipped control records as processed") &&
        expect_eq<std::uint64_t>(
            result.value().records_skipped,
            2,
            "snapshot and compaction records should be skipped") &&
        expect_eq<std::uint64_t>(
            result.value().last_sequence,
            3,
            "last sequence should include final put") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            1,
            "one live key should remain");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_recover_into_existing_table()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    if (!write_records(
            config.wal_path,
            {make_put_record(2, "v1|5:hello", "new")}))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    memtable::MemTable table;

    auto existing = table.put(
        "v1|5:hello",
        "old",
        1,
        100);

    if (!expect_true(existing.is_ok(), "existing memtable put should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    if (!expect_true(result.is_ok(), "recovery should succeed"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    auto entry = table.get("v1|5:hello");

    if (!expect_true(entry.has_value(), "key should exist after recovery"))
    {
      std::filesystem::remove_all(root);
      return false;
    }

    const bool ok =
        expect_true(
            entry->value == bytes("new"),
            "recovered newer record should replace existing table value") &&
        expect_eq<std::uint64_t>(
            entry->sequence,
            2,
            "recovered newer sequence should replace existing sequence") &&
        expect_eq<std::uint64_t>(
            result.value().live_keys,
            1,
            "live_keys should include final table state");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_empty_recovery_path_is_rejected()
  {
    memtable::MemTable table;
    wal::KvWalRecovery recovery{std::filesystem::path{}};

    auto result = recovery.recover(table);

    return expect_true(
               result.is_err(),
               "empty recovery path should fail") &&
           expect_error_code(
               result.error().code(),
               core::KvErrorCode::WalError,
               "empty recovery path should return WalError");
  }

  bool test_corrupted_wal_returns_error()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    std::filesystem::create_directories(config.wal_path.parent_path());

    {
      std::ofstream stream(
          config.wal_path,
          std::ios::binary | std::ios::out | std::ios::trunc);

      stream.put(static_cast<char>(0x01));
      stream.put(static_cast<char>(0x02));
      stream.put(static_cast<char>(0x03));
    }

    memtable::MemTable table;
    wal::KvWalRecovery recovery{config};

    auto result = recovery.recover(table);

    const bool ok =
        expect_true(
            result.is_err(),
            "corrupted WAL should fail recovery") &&
        expect_error_code(
            result.error().code(),
            core::KvErrorCode::Corruption,
            "corrupted WAL should return Corruption");

    std::filesystem::remove_all(root);

    return ok;
  }

  bool test_path_accessor()
  {
    const auto root = make_test_root();

    auto config = core::KvConfig::durable(root);

    wal::KvWalRecovery recovery{config};

    const bool ok = expect_eq<std::filesystem::path>(
        recovery.path(),
        config.wal_path,
        "recovery.path should return configured WAL path");

    std::filesystem::remove_all(root);

    return ok;
  }
}

int main()
{
  if (!test_recover_missing_wal_is_ok())
  {
    return 1;
  }

  if (!test_recover_single_put_record())
  {
    return 1;
  }

  if (!test_recover_multiple_put_records())
  {
    return 1;
  }

  if (!test_recover_delete_creates_tombstone())
  {
    return 1;
  }

  if (!test_recover_latest_sequence_wins())
  {
    return 1;
  }

  if (!test_recover_put_after_delete_restores_live_key())
  {
    return 1;
  }

  if (!test_recover_skips_snapshot_and_compaction_records())
  {
    return 1;
  }

  if (!test_recover_into_existing_table())
  {
    return 1;
  }

  if (!test_empty_recovery_path_is_rejected())
  {
    return 1;
  }

  if (!test_corrupted_wal_returns_error())
  {
    return 1;
  }

  if (!test_path_accessor())
  {
    return 1;
  }

  std::cout << "kv_test_wal_recovery passed\n";
  return 0;
}
