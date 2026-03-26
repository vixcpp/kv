/**
 *
 *  @file durable_restart.cpp
 *
 *  Demonstrates durability across restarts
 *
 */

#include <iostream>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/values/KvValue.hpp>

int main()
{
  using namespace vix::kv;

  auto db = open();

  std::cout << "Opening KV..." << std::endl;
  std::cout << "Writing value..." << std::endl;

  db.set({"session", "token"}, values::KvValue("abc123"));

  std::cout << "Restart program and run again to verify persistence." << std::endl;

  auto value = db.get({"session", "token"});

  if (value)
  {
    std::cout << "Recovered value: " << value->to_string() << std::endl;
  }
  else
  {
    std::cout << "No value found." << std::endl;
  }

  return 0;
}
