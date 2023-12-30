#include "blocks.h"

#include <cstring>
#include <sstream>
#include <iomanip>

InMemoryBlockDevice::InMemoryBlockDevice(block_idx_t block_count) {
    block_count_ = block_count;
    blocks_ = new uint8_t[block_count * BLOCK_SIZE];
}

InMemoryBlockDevice::~InMemoryBlockDevice() {
    delete[] blocks_;
}

void InMemoryBlockDevice::read_block(block_idx_t block_idx, uint8_t* buffer) {
    std::memcpy(buffer, blocks_ + block_idx * BLOCK_SIZE, BLOCK_SIZE);
}

void InMemoryBlockDevice::write_block(block_idx_t block_idx, const uint8_t* buffer) {
    std::memcpy(blocks_ + block_idx * BLOCK_SIZE, buffer, BLOCK_SIZE);
}

block_idx_t InMemoryBlockDevice::get_block_count() {
    return block_count_;
}

std::string BlockDevice::dump_blocks(block_idx_t start, block_idx_t end) {
    std::stringstream result;
    result << std::hex << std::setfill('0');
    for (block_idx_t i = start; i < end; i++) {
        uint8_t buffer[BLOCK_SIZE];
        read_block(i, buffer);
        result << std::endl << "Block " << std::setw(3) << i << ": " << std::endl;
        const int row_len = 32;
        for (int row = 0; row < BLOCK_SIZE / row_len; row++) {
            result << std::setw(3) << row * row_len << ": ";
            std::string repr = "";
            for (int col = 0; col < row_len; col++) {
                int v = (int)buffer[row * row_len + col];
                result << std::setw(2) << v << " ";
                if (v >= 32 && v <= 126) {
                    repr += (char)v;
                } else {
                    repr += ".";
                }
            }
            result << " |" << repr << "|" << std::endl;
        }
    }
    return result.str();
}
