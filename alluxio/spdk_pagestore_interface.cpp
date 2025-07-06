// spdk_pagestore_interface.cpp
#include "spdk_pagestore_interface.h"

SpdkPageStore::~SpdkPageStore() {
    if (channel_) spdk_put_io_channel(channel_);
    if (desc_) spdk_bdev_close(desc_);
    if (metadata_buf_) spdk_free(metadata_buf_);
}

bool SpdkPageStore::Init(const std::string& bdevName) {
    if (spdk_bdev_open_ext(bdevName.c_str(), true, nullptr, nullptr, &desc_) != 0) {
        std::cerr << "SPDK: Failed to open bdev " << bdevName << std::endl;
        return false;
    }

    bdev_ = spdk_bdev_desc_get_bdev(desc_);
    channel_ = spdk_bdev_get_io_channel(desc_);
    if (!channel_) {
        std::cerr << "SPDK: Failed to get I/O channel" << std::endl;
        spdk_bdev_close(desc_);
        desc_ = nullptr;
        return false;
    }

    metadata_buf_ = spdk_zmalloc(kMetadataSize, kPageSize, nullptr,
                                 SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    if (!metadata_buf_) {
        std::cerr << "SPDK: Failed to allocate metadata buffer" << std::endl;
        return false;
    }

    int rc = spdk_bdev_read(desc_, channel_, metadata_buf_, 0, kMetadataSize,
                            OnMetadataRead, this);
    if (rc != 0) {
        std::cerr << "SPDK: Failed to submit metadata read" << std::endl;
        return false;
    }

    page_used_.reset(); // Simplified
    return true;
}


void SpdkPageStore::WritePage(uint64_t pageId, const void* data, IoCallback cb) {
    if (pageId >= kMaxPages) {
        cb(false);
        return;
    }
    uint64_t offset = kMetadataSize + pageId * kPageSize;
    void* buf = spdk_zmalloc(kPageSize, kPageSize, nullptr,
                             SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    memcpy(buf, data, kPageSize);

    {
        std::lock_guard<std::mutex> lock(meta_mutex_);
        page_used_.set(pageId);
    }

    int rc = spdk_bdev_write(desc_, channel_, buf, offset, kPageSize,
                             OnWriteComplete, new std::pair<IoCallback, void*>(cb, buf));
    if (rc != 0) {
        cb(false);
        spdk_free(buf);
    }
}

void SpdkPageStore::ReadPage(uint64_t pageId, void* buffer, IoCallback cb) {
    if (pageId >= kMaxPages) {
        cb(false);
        return;
    }
    uint64_t offset = kMetadataSize + pageId * kPageSize;
    int rc = spdk_bdev_read(desc_, channel_, buffer, offset, kPageSize,
                            OnReadComplete, new IoCallback(cb));
    if (rc != 0) {
        cb(false);
    }
}

void SpdkPageStore::Flush(IoCallback cb) {
    int rc = spdk_bdev_flush(desc_, channel_, 0,
                             spdk_bdev_get_num_blocks(bdev_) * spdk_bdev_get_block_size(bdev_),
                             OnFlushComplete, new IoCallback(cb));
    if (rc != 0) {
        cb(false);
    }
}

void SpdkPageStore::OnWriteComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg) {
    auto* pair = static_cast<std::pair<IoCallback, void*>*>(cb_arg);
    pair->first(success);
    spdk_free(pair->second);
    delete pair;
    spdk_bdev_free_io(bdev_io);
}

void SpdkPageStore::OnReadComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg) {
    auto* cb = static_cast<IoCallback*>(cb_arg);
    (*cb)(success);
    delete cb;
    spdk_bdev_free_io(bdev_io);
}

void SpdkPageStore::OnFlushComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg) {
    auto* cb = static_cast<IoCallback*>(cb_arg);
    (*cb)(success);
    delete cb;
    spdk_bdev_free_io(bdev_io);
}

void SpdkPageStore::OnMetadataRead(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg) {
    auto* self = static_cast<SpdkPageStore*>(cb_arg);
    if (!success) {
        std::cerr << "SPDK: Metadata read failed" << std::endl;
    } else {
        // TODO: parse metadata_buf_ to fill page_used_
        self->page_used_.reset(); // simplified
    }
    spdk_bdev_free_io(bdev_io);
}
