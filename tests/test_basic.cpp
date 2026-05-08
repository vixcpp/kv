#include <vix/kv/api/Kv.hpp>
#include <vix/kv/api/Open.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
  namespace fs = std::filesystem;

  fs::path makeTestDirectory()
  {
    auto dir = fs::temp_directory_path() / "vix_kv_test_basic";

    fs::remove_all(dir);
    fs::create_directories(dir);

    return dir;
  }

  bool testPutAndGet()
  {
    const auto dir = makeTestDirectory();

    auto kv = vix::kv::open(dir.string());

    kv.put("hello", "world");

    const auto value = kv.get("hello");

    if (!value.has_value())
    {
      std::cerr << "expected key 'hello' to exist\n";
      return false;
    }

    if (*value != "world")
    {
      std::cerr << "expected value 'world', got '" << *value << "'\n";
      return false;
    }

    fs::remove_all(dir);
    return true;
  }
}

int main()
{
  if (!testPutAndGet())
  {
    return 1;
  }

  std::cout << "kv_test_basic passed\n";
  return 0;
}
