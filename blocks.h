#ifndef BLOCKS_H
#define BLOCKS_H

#include <cstddef>
#include <cstdint>
#include <string>

#define BLOCK_SIZE 512

#define DEVICE_SIZE_UNIT (8 * BLOCK_SIZE * BLOCK_SIZE)

using block_idx_t = uint32_t;

class BlockDevice {
    public:
    virtual void read_block(block_idx_t block_idx, uint8_t* buffer) = 0;
    virtual void write_block(block_idx_t block_idx, const uint8_t* buffer) = 0;
    virtual block_idx_t get_block_count() = 0;
    virtual std::string dump_blocks(block_idx_t start, block_idx_t end);
};

class InMemoryBlockDevice : public BlockDevice {
    public:
    InMemoryBlockDevice(block_idx_t block_count);
    ~InMemoryBlockDevice();

    void read_block(block_idx_t block_idx, uint8_t* buffer) override;
    void write_block(block_idx_t block_idx, const uint8_t* buffer) override;
    block_idx_t get_block_count() override;

    private:
    block_idx_t block_count_;
    uint8_t* blocks_;
};

#endif
