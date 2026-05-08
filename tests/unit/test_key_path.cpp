/**
 *
 *  @file test_key_path.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  KeyPath unit tests
 *
 */

#include <vix/kv/keys/KeyPath.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
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

  bool test_default_constructor_creates_empty_path()
  {
    keys::KeyPath path;

    return expect_true(
               path.empty(),
               "default KeyPath should be empty") &&
           expect_eq<std::size_t>(
               path.size(),
               0,
               "default KeyPath size should be 0") &&
           expect_eq<std::size_t>(
               path.byte_size(),
               0,
               "default KeyPath byte_size should be 0");
  }

  bool test_initializer_list_string_view_constructor()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"1"},
        std::string_view{"name"}};

    return expect_true(
               !path.empty(),
               "initializer-list KeyPath should not be empty") &&
           expect_eq<std::size_t>(
               path.size(),
               3,
               "initializer-list KeyPath should have 3 segments") &&
           expect_eq<std::string>(
               path.at(0),
               "users",
               "first segment should match") &&
           expect_eq<std::string>(
               path.at(1),
               "1",
               "second segment should match") &&
           expect_eq<std::string>(
               path.at(2),
               "name",
               "third segment should match");
  }

  bool test_initializer_list_string_constructor()
  {
    keys::KeyPath path{
        std::string{"settings"},
        std::string{"theme"}};

    return expect_eq<std::size_t>(
               path.size(),
               2,
               "string initializer-list should create 2 segments") &&
           expect_eq<std::string>(
               path.front(),
               "settings",
               "front segment should match") &&
           expect_eq<std::string>(
               path.back(),
               "theme",
               "back segment should match");
  }

  bool test_from_single_segment()
  {
    auto path = keys::KeyPath::from("hello");

    return expect_eq<std::size_t>(
               path.size(),
               1,
               "KeyPath::from should create one segment") &&
           expect_eq<std::string>(
               path.front(),
               "hello",
               "KeyPath::from segment should match");
  }

  bool test_from_segments()
  {
    std::vector<std::string> parts{
        "users",
        "42",
        "profile"};

    auto path = keys::KeyPath::from_segments(std::move(parts));

    return expect_eq<std::size_t>(
               path.size(),
               3,
               "from_segments should preserve segment count") &&
           expect_eq<std::string>(
               path.at(0),
               "users",
               "from_segments first segment should match") &&
           expect_eq<std::string>(
               path.at(1),
               "42",
               "from_segments second segment should match") &&
           expect_eq<std::string>(
               path.at(2),
               "profile",
               "from_segments third segment should match");
  }

  bool test_push_back_string_view()
  {
    keys::KeyPath path;

    path.push_back("users");
    path.push_back("1");

    return expect_eq<std::size_t>(
               path.size(),
               2,
               "push_back string_view should append segments") &&
           expect_eq<std::string>(
               path.at(0),
               "users",
               "first pushed segment should match") &&
           expect_eq<std::string>(
               path.at(1),
               "1",
               "second pushed segment should match");
  }

  bool test_push_back_string_move()
  {
    keys::KeyPath path;

    std::string segment = "name";
    path.push_back(std::move(segment));

    return expect_eq<std::size_t>(
               path.size(),
               1,
               "push_back string should append one segment") &&
           expect_eq<std::string>(
               path.back(),
               "name",
               "moved string segment should match");
  }

  bool test_append_segment()
  {
    keys::KeyPath path;

    path.append("users")
        .append("1")
        .append("name");

    return expect_eq<std::size_t>(
               path.size(),
               3,
               "append segment should be chainable") &&
           expect_eq<std::string>(
               path.at(0),
               "users",
               "append first segment should match") &&
           expect_eq<std::string>(
               path.at(1),
               "1",
               "append second segment should match") &&
           expect_eq<std::string>(
               path.at(2),
               "name",
               "append third segment should match");
  }

  bool test_append_other_path()
  {
    keys::KeyPath base{
        std::string_view{"users"}};

    keys::KeyPath suffix{
        std::string_view{"1"},
        std::string_view{"profile"}};

    base.append(suffix);

    return expect_eq<std::size_t>(
               base.size(),
               3,
               "append KeyPath should append all segments") &&
           expect_eq<std::string>(
               base.at(0),
               "users",
               "base segment should remain first") &&
           expect_eq<std::string>(
               base.at(1),
               "1",
               "appended first suffix should match") &&
           expect_eq<std::string>(
               base.at(2),
               "profile",
               "appended second suffix should match");
  }

  bool test_pop_back()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"1"},
        std::string_view{"name"}};

    path.pop_back();

    const bool after_one_pop =
        path.size() == 2 &&
        path.back() == "1";

    path.pop_back();
    path.pop_back();
    path.pop_back();

    return expect_true(
               after_one_pop,
               "pop_back should remove the last segment") &&
           expect_true(
               path.empty(),
               "pop_back on empty path should be safe");
  }

  bool test_byte_size()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"42"},
        std::string_view{"name"}};

    return expect_eq<std::size_t>(
        path.byte_size(),
        11,
        "byte_size should equal the sum of segment byte sizes");
  }

  bool test_at_mutable_access()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"1"}};

    path.at(1) = "2";

    return expect_eq<std::string>(
        path.at(1),
        "2",
        "mutable at() should allow segment modification");
  }

  bool test_front_and_back_mutable_access()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"1"}};

    path.front() = "customers";
    path.back() = "42";

    return expect_eq<std::string>(
               path.front(),
               "customers",
               "mutable front() should update first segment") &&
           expect_eq<std::string>(
               path.back(),
               "42",
               "mutable back() should update last segment");
  }

  bool test_segments_access()
  {
    keys::KeyPath path{
        std::string_view{"a"},
        std::string_view{"b"}};

    path.segments().push_back("c");

    const auto &segments = path.segments();

    return expect_eq<std::size_t>(
               segments.size(),
               3,
               "mutable segments() should allow appending") &&
           expect_eq<std::string>(
               segments[2],
               "c",
               "new segment should be visible through const segments()");
  }

  bool test_clear()
  {
    keys::KeyPath path{
        std::string_view{"users"},
        std::string_view{"1"}};

    path.clear();

    return expect_true(
               path.empty(),
               "clear should remove all segments") &&
           expect_eq<std::size_t>(
               path.size(),
               0,
               "clear should reset size to 0");
  }

  bool test_equality_and_inequality()
  {
    keys::KeyPath left{
        std::string_view{"users"},
        std::string_view{"1"}};

    keys::KeyPath same{
        std::string_view{"users"},
        std::string_view{"1"}};

    keys::KeyPath different{
        std::string_view{"users"},
        std::string_view{"2"}};

    return expect_true(
               left == same,
               "operator== should return true for identical paths") &&
           expect_true(
               left != different,
               "operator!= should return true for different paths");
  }

  bool test_lexicographical_ordering()
  {
    keys::KeyPath first{
        std::string_view{"users"},
        std::string_view{"1"}};

    keys::KeyPath second{
        std::string_view{"users"},
        std::string_view{"2"}};

    return expect_true(
        first < second,
        "operator< should compare paths lexicographically");
  }

  bool test_at_out_of_range_throws()
  {
    keys::KeyPath path{
        std::string_view{"users"}};

    bool thrown = false;

    try
    {
      (void)path.at(1);
    }
    catch (const std::out_of_range &)
    {
      thrown = true;
    }

    return expect_true(
        thrown,
        "at() should throw std::out_of_range for invalid index");
  }
}

int main()
{
  if (!test_default_constructor_creates_empty_path())
  {
    return 1;
  }

  if (!test_initializer_list_string_view_constructor())
  {
    return 1;
  }

  if (!test_initializer_list_string_constructor())
  {
    return 1;
  }

  if (!test_from_single_segment())
  {
    return 1;
  }

  if (!test_from_segments())
  {
    return 1;
  }

  if (!test_push_back_string_view())
  {
    return 1;
  }

  if (!test_push_back_string_move())
  {
    return 1;
  }

  if (!test_append_segment())
  {
    return 1;
  }

  if (!test_append_other_path())
  {
    return 1;
  }

  if (!test_pop_back())
  {
    return 1;
  }

  if (!test_byte_size())
  {
    return 1;
  }

  if (!test_at_mutable_access())
  {
    return 1;
  }

  if (!test_front_and_back_mutable_access())
  {
    return 1;
  }

  if (!test_segments_access())
  {
    return 1;
  }

  if (!test_clear())
  {
    return 1;
  }

  if (!test_equality_and_inequality())
  {
    return 1;
  }

  if (!test_lexicographical_ordering())
  {
    return 1;
  }

  if (!test_at_out_of_range_throws())
  {
    return 1;
  }

  std::cout << "kv_test_key_path passed\n";
  return 0;
}
