/**
 *
 *  @file persistent.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Durable WAL-backed example
 *
 */

#include <iostream>

#include <vix/kv/kv.hpp>

int main()
{
  auto opened = vix::kv::open_durable("data/examples/persistent.kv");

  if (opened.is_err())
  {
    std::cerr << "failed to open durable KV: "
              << opened.error().message()
              << '\n';

    return 1;
  }

  auto db = opened.move_value();

  auto set_name = db.set(
      {"app", "name"},
      "Vix KV");

  if (set_name.is_err())
  {
    std::cerr << "failed to store app name: "
              << set_name.error().message()
              << '\n';

    return 1;
  }

  auto set_mode = db.set(
      {"app", "mode"},
      "durable");

  if (set_mode.is_err())
  {
    std::cerr << "failed to store app mode: "
              << set_mode.error().message()
              << '\n';

    return 1;
  }

  auto flush = db.flush();

  if (flush.is_err())
  {
    std::cerr << "failed to flush KV: "
              << flush.error().message()
              << '\n';

    return 1;
  }

  auto name = db.get({"app", "name"});

  if (name.is_err())
  {
    std::cerr << "failed to read app name: "
              << name.error().message()
              << '\n';

    return 1;
  }

  auto mode = db.get({"app", "mode"});

  if (mode.is_err())
  {
    std::cerr << "failed to read app mode: "
              << mode.error().message()
              << '\n';

    return 1;
  }

  std::cout << "app/name : "
            << name.value().to_string()
            << '\n';

  std::cout << "app/mode : "
            << mode.value().to_string()
            << '\n';

  auto stats = db.stats();

  std::cout << "keys     : "
            << stats.key_count
            << '\n';

  std::cout << "wal      : "
            << (stats.wal_enabled ? "enabled" : "disabled")
            << '\n';

  std::cout << "writes   : "
            << stats.write_count()
            << '\n';

  auto closed = db.close();

  if (closed.is_err())
  {
    std::cerr << "failed to close durable KV: "
              << closed.error().message()
              << '\n';

    return 1;
  }

  return 0;
}
