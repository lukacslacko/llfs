#ifndef LLFS_H
#define LLFS_H

#include <cstdint>
#include "blocks.h"

using file_size_t = uint32_t;

#define BLOCK_CONTENTS_SIZE (BLOCK_SIZE - sizeof(file_size_t) - sizeof(block_idx_t))

struct Block {
    file_size_t remaining_size;
    block_idx_t next_block;
    std::byte data[BLOCK_CONTENTS_SIZE];
};

class ReadableFile {
    public:
    ReadableFile(block_idx_t first_block_idx, BlockDevice* block_device);
    ~ReadableFile();

    int read(std::byte* buffer, int bytes_to_read);
    bool eof() const;

    private:
    BlockDevice* block_device_;
    Block current_block_;
    int current_block_offset_;
};

#endif
