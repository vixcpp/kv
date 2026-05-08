/**
 *
 *  @file list.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Prefix listing example
 *
 */

#include <iostream>
#include <string>

#include <vix/kv/kv.hpp>

namespace
{
  int fail(const std::string &message)
  {
    std::cerr << message << '\n';
    return 1;
  }

  void print_entries(const vix::kv::Kv::ListResult &entries)
  {
    for (const auto &[key, value] : entries)
    {
      std::cout << "key   : ";

      const auto &segments = key.segments();

      for (std::size_t index = 0; index < segments.size(); ++index)
      {
        if (index > 0)
        {
          std::cout << "/";
        }

        std::cout << segments[index];
      }

      std::cout << '\n';

      std::cout << "value : "
                << value.to_string()
                << "\n\n";
    }
  }
}

int main()
{
  auto opened = vix::kv::open_memory();

  if (opened.is_err())
  {
    return fail(
        "failed to open KV: " + opened.error().message());
  }

  auto db = opened.move_value();

  auto set_user_1 = db.set({"users", "1", "name"}, "Ada");

  if (set_user_1.is_err())
  {
    return fail(
        "failed to store first user: " +
        set_user_1.error().message());
  }

  auto set_user_2 = db.set({"users", "2", "name"}, "Grace");

  if (set_user_2.is_err())
  {
    return fail(
        "failed to store second user: " +
        set_user_2.error().message());
  }

  auto set_config = db.set({"settings", "theme"}, "dark");

  if (set_config.is_err())
  {
    return fail(
        "failed to store settings: " +
        set_config.error().message());
  }

  auto users = db.list({"users"});

  if (users.is_err())
  {
    return fail(
        "failed to list users: " +
        users.error().message());
  }

  std::cout << "users entries\n";
  std::cout << "-------------\n";
  print_entries(users.value());

  std::cout << "total keys: "
            << db.size()
            << '\n';

  auto closed = db.close();

  if (closed.is_err())
  {
    return fail(
        "failed to close KV: " +
        closed.error().message());
  }

  return 0;
}
