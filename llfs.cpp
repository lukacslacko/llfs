#include "llfs.h"

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
