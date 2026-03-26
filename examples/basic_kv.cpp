/**
 *
 *  @file basic_kv.cpp
 *
 *  Vix KV - Basic example
 *
 */

#include <iostream>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/keys/KeyPath.hpp>
#include <vix/kv/values/KvValue.hpp>

int main()
{
  using namespace vix::kv;
  using keys::KeyPath;
  using values::KvValue;

  std::cout << "Opening KV..." << std::endl;

  auto db = open();

  std::cout << "Setting values..." << std::endl;

  db.set({"users", "1"}, KvValue("Alice"));
  db.set({"users", "2"}, KvValue("Bob"));
  db.set({"users", "3"}, KvValue("Charlie"));

  std::cout << "Reading values..." << std::endl;

  auto v1 = db.get({"users", "1"});
  if (v1)
  {
    std::cout << "users/1 = " << v1->to_string() << std::endl;
  }

  auto v2 = db.get({"users", "2"});
  if (v2)
  {
    std::cout << "users/2 = " << v2->to_string() << std::endl;
  }

  std::cout << "Listing users..." << std::endl;

  auto users = db.list({"users"});

  for (const auto &[key, value] : users)
  {
    std::cout << "key: ";

    for (const auto &segment : key.segments())
    {
      std::cout << segment << "/";
    }

    std::cout << " => " << value.to_string() << std::endl;
  }

  std::cout << "Deleting users/2..." << std::endl;

  db.erase({"users", "2"});

  auto after_delete = db.get({"users", "2"});
  if (!after_delete)
  {
    std::cout << "users/2 deleted successfully" << std::endl;
  }

  std::cout << "Done." << std::endl;

  return 0;
}
