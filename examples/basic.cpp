/**
 *
 *  @file basic.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Basic memory-only example
 *
 */

#include <iostream>

#include <vix/kv/kv.hpp>

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    std::cerr << "failed to open KV: "
              << opened.error().message()
              << '\n';

    return 1;
  }

  auto db = opened.move_value();

  auto set_result = db.set(
      {"users", "1", "name"},
      "Ada");

  if (set_result.is_err())
  {
    std::cerr << "failed to set value: "
              << set_result.error().message()
              << '\n';

    return 1;
  }

  auto value = db.get({"users", "1", "name"});

  if (value.is_err())
  {
    std::cerr << "failed to get value: "
              << value.error().message()
              << '\n';

    return 1;
  }

  std::cout << "key   : users/1/name\n";
  std::cout << "value : "
            << value.value().to_string()
            << '\n';

  std::cout << "size  : "
            << db.size()
            << '\n';

  if (db.contains({"users", "1", "name"}))
  {
    std::cout << "status: key exists\n";
  }

  auto entries = db.list({"users"});

  if (entries.is_err())
  {
    std::cerr << "failed to list values: "
              << entries.error().message()
              << '\n';

    return 1;
  }

  std::cout << "list  : "
            << entries.value().size()
            << " entrie(s)\n";

  auto closed = db.close();

  if (closed.is_err())
  {
    std::cerr << "failed to close KV: "
              << closed.error().message()
              << '\n';

    return 1;
  }

  return 0;
}
