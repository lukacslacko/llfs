#ifndef BLOCKS_H
#define BLOCKS_H

#include <cstddef>
#include <cstdint>

#define BLOCK_SIZE 512

using block_idx_t = uint32_t;

class BlockDevice {
    public:
    virtual void read_block(block_idx_t block_idx, std::byte* buffer) = 0;
    virtual void write_block(block_idx_t block_idx, const std::byte* buffer) = 0;
};

#endif
