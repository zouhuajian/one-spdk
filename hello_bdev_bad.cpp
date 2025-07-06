/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright (C) 2018 Intel Corporation.
 * All rights reserved.
 */

#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/bdev_zone.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

class HelloBdev {
public:
    explicit HelloBdev(std::string bdev_name = "Malloc0") : bdev_name_(std::move(bdev_name)) {}

    // 启动函数，相当于 hello_start
    void start() {
        spdk_app_opts_init(&opts_, sizeof(opts_));
        opts_.name = "hello_bdev";
        opts_.rpc_addr = nullptr;

        SPDK_NOTICELOG("Successfully started the application\n");

        int rc = spdk_bdev_open_ext(bdev_name_.c_str(), true, hello_bdev_event_cb, nullptr, &bdev_desc_);
        if (rc != 0) {
            SPDK_ERRLOG("Could not open bdev: %s\n", bdev_name_.c_str());
            spdk_app_stop(-1);
            return;
        }

        bdev_ = spdk_bdev_desc_get_bdev(bdev_desc_);
        bdev_io_channel_ = spdk_bdev_get_io_channel(bdev_desc_);
        if (bdev_io_channel_ == nullptr) {
            SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
            spdk_bdev_close(bdev_desc_);
            spdk_app_stop(-1);
            return;
        }

        buff_size_ = spdk_bdev_get_block_size(bdev_) * spdk_bdev_get_write_unit_size(bdev_);
        size_t buf_align = spdk_bdev_get_buf_align(bdev_);
        buff_ = static_cast<char*>(spdk_dma_zmalloc(buff_size_, buf_align, nullptr));
        if (buff_ == nullptr) {
            SPDK_ERRLOG("Failed to allocate buffer\n");
            cleanup();
            spdk_app_stop(-1);
            return;
        }

        std::snprintf(buff_, buff_size_, "%s", "Hello World!\n");

        if (spdk_bdev_is_zoned(bdev_)) {
            hello_reset_zone(this);
            // zoned bdev 调用完毕后会回调 reset_zone_complete -> hello_write
            return;
        }

        hello_write(this);
    }

    ~HelloBdev() {
        cleanup();
    }

    // 用于解析命令行参数的静态方法
    static int parse_arg(int ch, char *arg) {
        switch (ch) {
            case 'b':
                g_bdev_name_ = std::string(arg);
                return 0;
            default:
                return -EINVAL;
        }
    }

    // 打印帮助信息
    static void usage() {
        std::printf(" -b <bdev>                 name of the bdev to use\n");
    }

    // 入口，作为回调函数绑定，启动 SPDK 应用
    static void hello_start(void *arg) {
        auto *self = static_cast<HelloBdev*>(arg);
        self->start();
    }

    // 读取完成回调
    static void read_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
        auto *self = static_cast<HelloBdev*>(cb_arg);
        if (success) {
            SPDK_NOTICELOG("Read string from bdev : %s\n", self->buff_);
        } else {
            SPDK_ERRLOG("bdev io read error\n");
        }

        spdk_bdev_free_io(bdev_io);
        spdk_put_io_channel(self->bdev_io_channel_);
        spdk_bdev_close(self->bdev_desc_);
        SPDK_NOTICELOG("Stopping app\n");
        spdk_app_stop(success ? 0 : -1);
    }

    // 读请求
    static void hello_read(void *arg) {
        auto *self = static_cast<HelloBdev*>(arg);

        SPDK_NOTICELOG("Reading io\n");
        int rc = spdk_bdev_read(self->bdev_desc_, self->bdev_io_channel_,
                                self->buff_, 0, self->buff_size_, read_complete, self);

        if (rc == -ENOMEM) {
            SPDK_NOTICELOG("Queueing io\n");
            self->bdev_io_wait_.bdev = self->bdev_;
            self->bdev_io_wait_.cb_fn = hello_read;
            self->bdev_io_wait_.cb_arg = self;
            spdk_bdev_queue_io_wait(self->bdev_, self->bdev_io_channel_, &self->bdev_io_wait_);
        } else if (rc != 0) {
            SPDK_ERRLOG("%s error while reading from bdev: %d\n", spdk_strerror(-rc), rc);
            spdk_put_io_channel(self->bdev_io_channel_);
            spdk_bdev_close(self->bdev_desc_);
            spdk_app_stop(-1);
        }
    }

    // 写完成回调
    static void write_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
        auto *self = static_cast<HelloBdev*>(cb_arg);

        spdk_bdev_free_io(bdev_io);

        if (success) {
            SPDK_NOTICELOG("bdev io write completed successfully\n");
        } else {
            SPDK_ERRLOG("bdev io write error: %d\n", EIO);
            spdk_put_io_channel(self->bdev_io_channel_);
            spdk_bdev_close(self->bdev_desc_);
            spdk_app_stop(-1);
            return;
        }

        std::memset(self->buff_, 0, self->buff_size_);
        hello_read(self);
    }

    // 写请求
    static void hello_write(void *arg) {
        auto *self = static_cast<HelloBdev*>(arg);

        SPDK_NOTICELOG("Writing to the bdev\n");
        int rc = spdk_bdev_write(self->bdev_desc_, self->bdev_io_channel_,
                                 self->buff_, 0, self->buff_size_, write_complete, self);

        if (rc == -ENOMEM) {
            SPDK_NOTICELOG("Queueing io\n");
            self->bdev_io_wait_.bdev = self->bdev_;
            self->bdev_io_wait_.cb_fn = hello_write;
            self->bdev_io_wait_.cb_arg = self;
            spdk_bdev_queue_io_wait(self->bdev_, self->bdev_io_channel_, &self->bdev_io_wait_);
        } else if (rc != 0) {
            SPDK_ERRLOG("%s error while writing to bdev: %d\n", spdk_strerror(-rc), rc);
            spdk_put_io_channel(self->bdev_io_channel_);
            spdk_bdev_close(self->bdev_desc_);
            spdk_app_stop(-1);
        }
    }

    // bdev 事件回调
    static void hello_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev,
                                   void *event_ctx) {
        SPDK_NOTICELOG("Unsupported bdev event: type %d\n", type);
    }

    // reset zone 完成回调
    static void reset_zone_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
        auto *self = static_cast<HelloBdev*>(cb_arg);
        spdk_bdev_free_io(bdev_io);

        if (!success) {
            SPDK_ERRLOG("bdev io reset zone error: %d\n", EIO);
            spdk_put_io_channel(self->bdev_io_channel_);
            spdk_bdev_close(self->bdev_desc_);
            spdk_app_stop(-1);
            return;
        }

        hello_write(self);
    }

    // reset zone 请求
    static void hello_reset_zone(void *arg) {
        auto *self = static_cast<HelloBdev*>(arg);

        int rc = spdk_bdev_zone_management(self->bdev_desc_, self->bdev_io_channel_,
                                           0, SPDK_BDEV_ZONE_RESET, reset_zone_complete, self);

        if (rc == -ENOMEM) {
            SPDK_NOTICELOG("Queueing io\n");
            self->bdev_io_wait_.bdev = self->bdev_;
            self->bdev_io_wait_.cb_fn = hello_reset_zone;
            self->bdev_io_wait_.cb_arg = self;
            spdk_bdev_queue_io_wait(self->bdev_, self->bdev_io_channel_, &self->bdev_io_wait_);
        } else if (rc != 0) {
            SPDK_ERRLOG("%s error while resetting zone: %d\n", spdk_strerror(-rc), rc);
            spdk_put_io_channel(self->bdev_io_channel_);
            spdk_bdev_close(self->bdev_desc_);
            spdk_app_stop(-1);
        }
    }

    static std::string g_bdev_name_;

private:
    void cleanup() {
        if (buff_ != nullptr) {
            spdk_dma_free(buff_);
            buff_ = nullptr;
        }
        if (bdev_io_channel_ != nullptr) {
            spdk_put_io_channel(bdev_io_channel_);
            bdev_io_channel_ = nullptr;
        }
        if (bdev_desc_ != nullptr) {
            spdk_bdev_close(bdev_desc_);
            bdev_desc_ = nullptr;
        }
    }

    struct spdk_app_opts opts_{};
    struct spdk_bdev *bdev_{nullptr};
    struct spdk_bdev_desc *bdev_desc_{nullptr};
    struct spdk_io_channel *bdev_io_channel_{nullptr};
    struct spdk_bdev_io_wait_entry bdev_io_wait_{};

    char *buff_{nullptr};
    uint32_t buff_size_{0};

    std::string bdev_name_;
};

std::string HelloBdev::g_bdev_name_ = "Malloc0";

int main(int argc, char **argv) {
    HelloBdev hello(HelloBdev::g_bdev_name_);

    spdk_app_opts opts{};
    spdk_app_opts_init(&opts, sizeof(opts));
    opts.name = "hello_bdev";

    // 解析参数，传给静态函数
    int rc = spdk_app_parse_args(argc, argv, &opts, "b:", nullptr,
                                HelloBdev::parse_arg, HelloBdev::usage);
    if (rc != SPDK_APP_PARSE_ARGS_SUCCESS) {
        return rc;
    }

    hello = HelloBdev(HelloBdev::g_bdev_name_);

    rc = spdk_app_start(&opts, HelloBdev::hello_start, &hello);
    if (rc != 0) {
        SPDK_ERRLOG("ERROR starting application\n");
    }

    spdk_app_fini();
    return rc;
}
