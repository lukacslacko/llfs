#ifndef LLFS_H
#define LLFS_H

#include <cstdint>
#include <vector>
#include "blocks.h"

// MAGIC is low-endian "LLFS" in ASCII
#define MAGIC 0x5346464c

using file_size_t = uint32_t;

#define BLOCK_CONTENTS_SIZE (BLOCK_SIZE - sizeof(file_size_t) - sizeof(block_idx_t))

struct Block {
    file_size_t remaining_size;
    block_idx_t next_block;
    std::byte data[BLOCK_CONTENTS_SIZE];
};

struct FirstBlockData {
    uint32_t magic_number;
    uint32_t block_count;
};

struct FirstBlock {
    FirstBlockData data;
    std::byte padding[BLOCK_SIZE - sizeof(FirstBlockData)];
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

class BlockBitmap {
    public:
    BlockBitmap(BlockDevice* block_device);

    int get_free_block_count() const;

    block_idx_t allocate_block(bool flush = true);
    void release(block_idx_t block_idx, bool flush = true);
    void write_bitmap();

    private:
    BlockDevice* block_device_;
    FirstBlock first_block_;
    std::vector<bool> bitmap_;
};

class BlockyLLFS {
    public:
    BlockyLLFS(BlockDevice* block_device);
    ~BlockyLLFS();

    ReadableFile* open_file(block_idx_t first_block_idx);
    block_idx_t write_file(const std::byte* buffer, int bytes_to_write);
    void delete_file(block_idx_t first_block_idx);

    BlockDevice* get_block_device() const { return block_device_; }

    private:
    BlockDevice* block_device_;
    BlockBitmap* block_bitmap_;
};

struct DirectoryEntry {
    char name[60];

    // Zero means deleted, can be reused.
    block_idx_t first_block_idx;
};

struct DisplayedFileInfo {
    char name[60];
    file_size_t size;
};

class Directory {
    public:
    Directory(BlockyLLFS* blocky_llfs, block_idx_t first_block_idx);
    ReadableFile* find(const char* name);
    void add(const char* name, const std::byte* buffer, int bytes_to_write);
    void remove(const char* name);

    void write_entries();

    std::vector<DisplayedFileInfo> list();

    private:
    BlockyLLFS* blocky_llfs_;
    block_idx_t first_block_idx_;
    std::vector<DirectoryEntry> entries_;
};

#endif
