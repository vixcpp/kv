/**
 *
 *  @file prefix_scan.cpp
 *
 *  Demonstrates prefix listing
 *
 */

#include <iostream>

#include <vix/kv/api/Open.hpp>
#include <vix/kv/values/KvValue.hpp>

int main()
{
  using namespace vix::kv;

  auto db = open();

  db.set({"products", "1"}, values::KvValue("Laptop"));
  db.set({"products", "2"}, values::KvValue("Phone"));
  db.set({"products", "3"}, values::KvValue("Tablet"));

  db.set({"orders", "1"}, values::KvValue("OrderA"));
  db.set({"orders", "2"}, values::KvValue("OrderB"));

  std::cout << "Products:" << std::endl;

  auto products = db.list({"products"});

  for (const auto &[key, value] : products)
  {
    std::cout << "- " << value.to_string() << std::endl;
  }

  return 0;
}
