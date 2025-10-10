#include "efeum/mmapping.h"
#include "mio/mmap.hpp"
#include <filesystem>

extern "C" struct mmap_source_struct {
    mio::mmap_source* mmap;
};

mmap_source_t create_mmap_source(wchar_t const* filePath, size_t offset, size_t length) {
    auto absPath = std::filesystem::absolute(filePath);
    if (!std::filesystem::exists(filePath)) {
        return nullptr;
    }

    mmap_source_struct* mmap = new mmap_source_struct;
    mmap->mmap = new mio::mmap_source;
    std::error_code error;
    try {
        *mmap->mmap = mio::make_mmap_source(absPath.string(), 0, mio::map_entire_file, error);
    } catch (std::system_error const& e) {
        delete mmap->mmap;
        delete mmap;
        return nullptr;
    }

    if (error) {
        delete mmap->mmap;
        delete mmap;
        return nullptr;
    }

    return mmap;
}

void delete_mmap_source(mmap_source_t mmap) {
    if (mmap) {
        delete mmap->mmap;
        delete mmap;
    }
}

size_t get_mmap_size(mmap_source_t mmap) {
    if (!mmap || !mmap->mmap) {
        return 0;
    }
    return mmap->mmap->size();
}

char const* get_mmap_data(mmap_source_t mmap) {
    if (!mmap || !mmap->mmap) {
        return nullptr;
    }
    return mmap->mmap->data();
}
