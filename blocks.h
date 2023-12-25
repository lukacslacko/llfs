#ifndef BLOCKS_H
#define BLOCKS_H

#include <cstddef>
#include <cstdint>

#define BLOCK_SIZE 512

#define DEVICE_SIZE_UNIT (8 * BLOCK_SIZE * BLOCK_SIZE)

using block_idx_t = uint32_t;

class BlockDevice {
    public:
    virtual void read_block(block_idx_t block_idx, std::byte* buffer) = 0;
    virtual void write_block(block_idx_t block_idx, const std::byte* buffer) = 0;
    block_idx_t get_block_count() = 0;
};

class InMemoryBlockDevice : public BlockDevice {
    public:
    InMemoryBlockDevice(block_idx_t block_count);
    ~InMemoryBlockDevice();

    void read_block(block_idx_t block_idx, std::byte* buffer) override;
    void write_block(block_idx_t block_idx, const std::byte* buffer) override;
    block_idx_t get_block_count() override;

    private:
    block_idx_t block_count_;
    std::byte* blocks_;
};

#endif
