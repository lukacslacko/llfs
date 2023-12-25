#include <iostream>

#include "blocks.h"
#include "llfs.h"

int main() {
  InMemoryBlockDevice block_device(64 * 1024 * 1024);
  format(&block_device);
  LLFS llfs(&block_device);
  Directory* root = llfs.get_root_directory();
  auto foo = root->mkdir("foo");
  auto bar = foo->mkdir("bar");
  foo->write_file("baz", reinterpret_cast<const std::byte*>("hello"), 5);

  auto f = root->cd("foo")->cd("bar")->open_file("baz");
  std::byte buffer[5];
  f->read(buffer, 5);
  std::cout << reinterpret_cast<char*>(buffer) << std::endl;
}
