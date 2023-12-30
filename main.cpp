#include <iostream>

#include "blocks.h"
#include "llfs.h"

int main() {
  InMemoryBlockDevice block_device(4 * 1024);
  format(&block_device);
  LLFS llfs(&block_device);
  std::cout << block_device.dump_blocks(0, 6) << std::endl;
  Directory* root = llfs.get_root_directory();
  auto foo = root->mkdir("foo");
  std::cout << block_device.dump_blocks(0, 6) << std::endl;
  auto bar = foo->mkdir("bar");
  std::cout << block_device.dump_blocks(0, 6) << std::endl;
  bar->write_file("baz", reinterpret_cast<const uint8_t*>("hello"), 5);

  std::cout << block_device.dump_blocks(0, 6) << std::endl;

  auto f = root->cd("foo")->cd("bar")->open_file("baz");
  uint8_t buffer[5];
  f->read(buffer, 5);
  std::cout << reinterpret_cast<char*>(buffer) << std::endl;
}
