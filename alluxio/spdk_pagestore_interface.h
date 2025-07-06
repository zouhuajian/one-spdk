// SPDX-License-Identifier: Apache-2.0
// Alluxio-like SPDK-backed PageStore interface in C++ (No RocksDB version)
// Author: chatgpt/openai

#pragma once

#include <spdk/bdev.h>
#include <spdk/env.h>
#include <spdk/thread.h>
#include <spdk/log.h>
#include <memory>
#include <string>
#include <functional>
#include <bitset>
#include <mutex>
#include <iostream>
#include <cstring>

constexpr size_t kPageSize = 4096;
constexpr size_t kMetadataSize = 1024 * 1024;
constexpr size_t kMaxPages = (1024 * 1024 * 1024) / kPageSize; // 1GB space

struct PageMeta {
    uint32_t version;
    uint32_t crc32;
};

using IoCallback = std::function<void(bool)>;

class PageStore {
public:
    virtual ~PageStore() = default;
    virtual bool Init(const std::string& bdevName) = 0;
    virtual void WritePage(uint64_t pageId, const void* data, IoCallback cb) = 0;
    virtual void ReadPage(uint64_t pageId, void* buffer, IoCallback cb) = 0;
    virtual void Flush(IoCallback cb) = 0;
};

class SpdkPageStore : public PageStore {
public:
    SpdkPageStore() = default;
    ~SpdkPageStore() override;

    bool Init(const std::string& bdevName) override;
    void WritePage(uint64_t pageId, const void* data, IoCallback cb) override;
    void ReadPage(uint64_t pageId, void* buffer, IoCallback cb) override;
    void Flush(IoCallback cb) override;

private:
    struct spdk_bdev* bdev_ = nullptr;
    struct spdk_bdev_desc* desc_ = nullptr;
    struct spdk_io_channel* channel_ = nullptr;
    void* metadata_buf_ = nullptr;
    std::bitset<kMaxPages> page_used_;
    std::mutex meta_mutex_;

    static void OnWriteComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg);
    static void OnReadComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg);
    static void OnFlushComplete(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg);
    static void OnMetadataRead(struct spdk_bdev_io* bdev_io, bool success, void* cb_arg);
};

// Usage Example (demo.cpp):
//
// auto store = std::make_unique<SpdkPageStore>();
// if (store->Init("Nvme0n1")) {
//   char data[kPageSize] = "hello page";
//   store->WritePage(0, data, [](bool ok) {
//     if (ok) std::cout << "Write OK" << std::endl;
//   });
//   char buffer[kPageSize] = {0};
//   store->ReadPage(0, buffer, [](bool ok) {
//     if (ok) std::cout << "Read OK" << std::endl;
//   });
// }
