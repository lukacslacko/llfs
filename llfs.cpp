#include "llfs.h"

#include <stdio.h>

ReadableFile::ReadableFile(block_idx_t first_block_idx, BlockDevice* block_device) {
    block_device_ = block_device;
    block_device_->read_block(first_block_idx, reinterpret_cast<std::byte*>(&current_block_));
    current_block_offset_ = 0;
}

ReadableFile::~ReadableFile() {
}

int ReadableFile::read(std::byte* buffer, int bytes_to_read) {
    int bytes_read = 0;
    while (bytes_read < bytes_to_read && !eof()) {
        buffer[bytes_read] = current_block_.data[current_block_offset_];
        bytes_read++;
        current_block_offset_++;
        if (current_block_offset_ == BLOCK_CONTENTS_SIZE && !eof()) {
            block_device_->read_block(current_block_.next_block, reinterpret_cast<std::byte*>(&current_block_));
            current_block_offset_ = 0;
        }
    }
    return bytes_read;
}

bool ReadableFile::eof() const {
    return current_block_.remaining_size == current_block_offset_;
}

BlockBitmap::BlockBitmap(BlockDevice* block_device) {
    block_device_ = block_device;
    block_device_->read_block(1, reinterpret_cast<std::byte*>(&first_block_));
    std::byte buffer[BLOCK_SIZE];
    const int bitmap_num_blocks = first_block_.data.block_count / 8 / BLOCK_SIZE;
    for (int i = 0; i < bitmap_num_blocks; i++) {
        block_device_->read_block(2 + i, buffer);
        for (int j = 0; j < BLOCK_SIZE; j++) {
            for (int k = 0; k < 8; k++) {
                bitmap_.push_back(std::byte(0) != (buffer[j] & (std::byte(1) << k)));
            }
        }
    }
}

int BlockBitmap::get_free_block_count() const {
    int count = 0;
    for (bool b : bitmap_) {
        if (!b) {
            count++;
        }
    }
    return count;
}

void BlockBitmap::write_bitmap() {
    std::byte buffer[BLOCK_SIZE];
    const int bitmap_num_blocks = first_block_.data.block_count / 8 / BLOCK_SIZE;
    for (int i = 0; i < bitmap_num_blocks; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            buffer[j] = std::byte(0);
            for (int k = 0; k < 8; k++) {
                if (bitmap_[i * BLOCK_SIZE * 8 + j * 8 + k]) {
                    buffer[j] |= std::byte(1) << k;
                }
            }
        }
        block_device_->write_block(2 + i, buffer);
    }
}

block_idx_t BlockBitmap::allocate_block(bool flush) {
    block_idx_t block = 0;
    for (int i = 0; i < bitmap_.size(); i++) {
        if (!bitmap_[i]) {
            bitmap_[i] = true;
            block = i;
            break;
        }
    }
    if (flush) write_bitmap();
    return block;
}

void BlockBitmap::release(block_idx_t block_idx, bool flush) {
    bitmap_[block_idx] = false;
    if (flush) write_bitmap();
}

BlockyLLFS::BlockyLLFS(BlockDevice* block_device) {
    block_device_ = block_device;
    block_bitmap_ = new BlockBitmap(block_device_);
}

BlockyLLFS::~BlockyLLFS() {
    delete block_bitmap_;
}

ReadableFile* BlockyLLFS::open_file(block_idx_t first_block_idx) {
    return new ReadableFile(first_block_idx, block_device_);
}

block_idx_t BlockyLLFS::write_file(const std::byte* buffer, int bytes_to_write) {
    block_idx_t first_block_idx = block_bitmap_->allocate_block(false);
    block_idx_t current_block_idx = first_block_idx;
    int bytes_written = 0;
    while (bytes_written < bytes_to_write) {
        Block block;
        block.remaining_size = bytes_to_write - bytes_written;
        block.next_block = 0;
        for (int i = 0; i < BLOCK_CONTENTS_SIZE; i++) {
            if (bytes_written < bytes_to_write) {
                block.data[i] = buffer[bytes_written];
                bytes_written++;
            } else {
                block.data[i] = std::byte(0);
            }
        }
        if (bytes_written < bytes_to_write) {
            block.next_block = block_bitmap_->allocate_block(false);
        }
        block_device_->write_block(current_block_idx, reinterpret_cast<std::byte*>(&block));
        current_block_idx = block.next_block;
    }
    block_bitmap_->write_bitmap();
    return first_block_idx;
}

void BlockyLLFS::delete_file(block_idx_t first_block_idx) {
    Block block;
    block_idx_t current_block_idx = first_block_idx;
    while (current_block_idx != 0) {
        block_device_->read_block(current_block_idx, reinterpret_cast<std::byte*>(&block));
        block_bitmap_->release(current_block_idx, false);
        current_block_idx = block.next_block;
    }
    block_bitmap_->write_bitmap();
}

Directory::Directory(BlockyLLFS* blocky_llfs, block_idx_t first_block_idx) {
    blocky_llfs_ = blocky_llfs;
    first_block_idx_ = first_block_idx;
    ReadableFile* file = blocky_llfs_->open_file(first_block_idx);
    DirectoryEntry entry;
    while (file->read(reinterpret_cast<std::byte*>(&entry), sizeof(DirectoryEntry)) == sizeof(DirectoryEntry)) {
        entries_.push_back(entry);
    }
    delete file;
}

ReadableFile* Directory::open_file(const char* name) {
    for (DirectoryEntry entry : entries_) {
        if (strcmp(entry.name, name) == 0) {
            if (entry.is_directory) {
                printf("Cannot open directory %s as file\n", name);
                return 0;
            }
            return new ReadableFile(entry.first_block_idx, blocky_llfs_->get_block_device());
        }
    }
    return 0;
}

Directory* Directory::cd(const char* name) {
    for (DirectoryEntry entry : entries_) {
        if (strcmp(entry.name, name) == 0) {
            if (!entry.is_directory) {
                printf("Cannot cd into file %s\n", name);
                return 0;
            }
            return new Directory(blocky_llfs_, entry.first_block_idx);
        }
    }
    return 0;
}

void Directory::write_file(const char* name, const std::byte* buffer, int bytes_to_write) {
    DirectoryEntry entry;
    strcpy(entry.name, name);
    entry.is_directory = false;
    entry.first_block_idx = blocky_llfs_->write_file(buffer, bytes_to_write);
    entries_.push_back(entry);
    write_entries();
}

Directory* Directory::mkdir(const char* name) {
    DirectoryEntry entry;
    strcpy(entry.name, name);
    entry.is_directory = true;
    entry.first_block_idx = blocky_llfs_->write_file(0, 0);
    entries_.push_back(entry);
    write_entries();
    return new Directory(blocky_llfs_, entry.first_block_idx);
}

void Directory::remove(const char* name) {
    for (int i = 0; i < entries_.size(); i++) {
        if (strcmp(entries_[i].name, name) == 0) {
            blocky_llfs_->delete_file(entries_[i].first_block_idx);
            entries_.erase(entries_.begin() + i);
            break;
        }
    }
    write_entries();
}

void Directory::write_entries() {
    blocky_llfs_->delete_file(first_block_idx_);
    std::byte* buffer = new std::byte[entries_.size() * sizeof(DirectoryEntry)];
    for (int i = 0; i < entries_.size(); i++) {
        memcpy(buffer + i * sizeof(DirectoryEntry), &entries_[i], sizeof(DirectoryEntry));
    }
    first_block_idx_ = blocky_llfs_->write_file(buffer, entries_.size() * sizeof(DirectoryEntry));
    delete[] buffer;
}

std::vector<DisplayedFileInfo> Directory::list() {
    std::vector<DisplayedFileInfo> result;
    for (DirectoryEntry entry : entries_) {
        DisplayedFileInfo info;
        strcpy(info.name, entry.name);
        Block file_first_block;
        blocky_llfs_->get_block_device()->read_block(entry.first_block_idx, reinterpret_cast<std::byte*>(&file_first_block));
        info.size = file_first_block.remaining_size;
        result.push_back(info);
    }
    return result;
}

LLFS::LLFS(BlockDevice* block_device) {
    blocky_llfs_ = new BlockyLLFS(block_device);
    FirstBlock first_block;
    block_device->read_block(1, reinterpret_cast<std::byte*>(&first_block));
    root_directory_ = new Directory(blocky_llfs_, first_block.data.root_directory_idx);
}

LLFS::~LLFS() {
    delete root_directory_;
    delete blocky_llfs_;
}

BlockyLLFS* format(BlockDevice* block_device) {
    FirstBlock first_block;
    first_block.data.magic_number = MAGIC;
    first_block.data.block_count = block_device->get_block_count();
    int bitmap_num_blocks = first_block.data.block_count / 8 / BLOCK_SIZE;
    // Plus one is for the root directory.
    for (int i = 0; i < bitmap_num_blocks + 1; i++) {
        block_device->write_block(2 + i, reinterpret_cast<std::byte*>(new std::byte[BLOCK_SIZE]));
    }
    first_block.data.root_directory_idx = bitmap_num_blocks + 2;
    block_device->write_block(1, reinterpret_cast<std::byte*>(&first_block));
}
