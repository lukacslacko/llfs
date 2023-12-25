#include "blocks.h"

#include <cstring>

InMemoryBlockDevice::InMemoryBlockDevice(block_idx_t block_count) {
    block_count_ = block_count;
    blocks_ = new std::byte[block_count * BLOCK_SIZE];
}

InMemoryBlockDevice::~InMemoryBlockDevice() {
    delete[] blocks_;
}

void InMemoryBlockDevice::read_block(block_idx_t block_idx, std::byte* buffer) {
    std::memcpy(buffer, blocks_ + block_idx * BLOCK_SIZE, BLOCK_SIZE);
}

void InMemoryBlockDevice::write_block(block_idx_t block_idx, const std::byte* buffer) {
    std::memcpy(blocks_ + block_idx * BLOCK_SIZE, buffer, BLOCK_SIZE);
}

block_idx_t InMemoryBlockDevice::get_block_count() {
    return block_count_;
}
