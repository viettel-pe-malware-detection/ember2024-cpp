#pragma once

extern "C" {
    struct mmap_source_struct;

    typedef mmap_source_struct* mmap_source_t;

    mmap_source_t create_mmap_source(wchar_t const* filePath, size_t offset, size_t length);

    size_t get_mmap_size(mmap_source_t mmap);

    char const* get_mmap_data(mmap_source_t mmap);

    void delete_mmap_source(mmap_source_t mmap);
}
