/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (C) 2018 Intel Corporation.
 *   All rights reserved.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/bdev_zone.h"

static char *g_bdev_name = const_cast<char *>("Malloc0");

struct HelloContext {
    spdk_bdev *bdev{nullptr};
    spdk_bdev_desc *bdev_desc{nullptr};
    spdk_io_channel *bdev_io_channel{nullptr};
    char *buff{nullptr};
    uint32_t buff_size{0};
    char *bdev_name{nullptr};
    spdk_bdev_io_wait_entry bdev_io_wait{};
};

static void hello_bdev_usage() {
    std::printf(" -b <bdev>                 name of the bdev to use\n");
}

static int hello_bdev_parse_arg(int ch, char *arg) {
    switch (ch) {
        case 'b':
            g_bdev_name = arg;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static void read_complete(spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto *ctx = static_cast<HelloContext *>(cb_arg);

    if (success) {
        SPDK_NOTICELOG("Read string from bdev : %s\n", ctx->buff);
    } else {
        SPDK_ERRLOG("bdev io read error\n");
    }

    spdk_bdev_free_io(bdev_io);
    spdk_put_io_channel(ctx->bdev_io_channel);
    spdk_bdev_close(ctx->bdev_desc);
    SPDK_NOTICELOG("Stopping app\n");
    spdk_app_stop(success ? 0 : -1);
}

static void hello_read(void *arg) {
    auto *ctx = static_cast<HelloContext *>(arg);
    int rc = spdk_bdev_read(ctx->bdev_desc, ctx->bdev_io_channel, ctx->buff, 0,
                            ctx->buff_size, read_complete, ctx);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        ctx->bdev_io_wait.bdev = ctx->bdev;
        ctx->bdev_io_wait.cb_fn = hello_read;
        ctx->bdev_io_wait.cb_arg = ctx;
        spdk_bdev_queue_io_wait(ctx->bdev, ctx->bdev_io_channel, &ctx->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while reading from bdev: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
    }
}

static void write_complete(spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto *ctx = static_cast<HelloContext *>(cb_arg);
    spdk_bdev_free_io(bdev_io);

    if (success) {
        SPDK_NOTICELOG("bdev io write completed successfully\n");
    } else {
        SPDK_ERRLOG("bdev io write error: %d\n", EIO);
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    std::memset(ctx->buff, 0, ctx->buff_size);
    hello_read(ctx);
}

static void hello_write(void *arg) {
    auto *ctx = static_cast<HelloContext *>(arg);
    int rc = spdk_bdev_write(ctx->bdev_desc, ctx->bdev_io_channel, ctx->buff, 0,
                             ctx->buff_size, write_complete, ctx);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        ctx->bdev_io_wait.bdev = ctx->bdev;
        ctx->bdev_io_wait.cb_fn = hello_write;
        ctx->bdev_io_wait.cb_arg = ctx;
        spdk_bdev_queue_io_wait(ctx->bdev, ctx->bdev_io_channel, &ctx->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while writing to bdev: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
    }
}

static void hello_bdev_event_cb(enum spdk_bdev_event_type type, spdk_bdev *bdev, void *event_ctx) {
    SPDK_NOTICELOG("Unsupported bdev event: type %d\n", type);
}

static void reset_zone_complete(spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto *ctx = static_cast<HelloContext *>(cb_arg);
    spdk_bdev_free_io(bdev_io);

    if (!success) {
        SPDK_ERRLOG("bdev io reset zone error: %d\n", EIO);
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    hello_write(ctx);
}

static void hello_reset_zone(void *arg) {
    auto *ctx = static_cast<HelloContext *>(arg);
    int rc = spdk_bdev_zone_management(ctx->bdev_desc, ctx->bdev_io_channel, 0,
                                       SPDK_BDEV_ZONE_RESET, reset_zone_complete, ctx);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        ctx->bdev_io_wait.bdev = ctx->bdev;
        ctx->bdev_io_wait.cb_fn = hello_reset_zone;
        ctx->bdev_io_wait.cb_arg = ctx;
        spdk_bdev_queue_io_wait(ctx->bdev, ctx->bdev_io_channel, &ctx->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while resetting zone: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
    }
}

static void hello_start(void *arg1) {
    auto *ctx = static_cast<HelloContext *>(arg1);
    uint32_t buf_align = 0;
    int rc = 0;

    ctx->bdev = nullptr;
    ctx->bdev_desc = nullptr;

    SPDK_NOTICELOG("Successfully started the application\n");
    SPDK_NOTICELOG("Opening the bdev %s\n", ctx->bdev_name);

    rc = spdk_bdev_open_ext(ctx->bdev_name, true, hello_bdev_event_cb, nullptr, &ctx->bdev_desc);
    if (rc) {
        SPDK_ERRLOG("Could not open bdev: %s\n", ctx->bdev_name);
        spdk_app_stop(-1);
        return;
    }

    ctx->bdev = spdk_bdev_desc_get_bdev(ctx->bdev_desc);

    SPDK_NOTICELOG("Opening io channel\n");
    ctx->bdev_io_channel = spdk_bdev_get_io_channel(ctx->bdev_desc);
    if (!ctx->bdev_io_channel) {
        SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    ctx->buff_size = spdk_bdev_get_block_size(ctx->bdev) * spdk_bdev_get_write_unit_size(ctx->bdev);
    buf_align = spdk_bdev_get_buf_align(ctx->bdev);
    ctx->buff = static_cast<char *>(spdk_dma_zmalloc(ctx->buff_size, buf_align, nullptr));
    if (!ctx->buff) {
        SPDK_ERRLOG("Failed to allocate buffer\n");
        spdk_put_io_channel(ctx->bdev_io_channel);
        spdk_bdev_close(ctx->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    std::snprintf(ctx->buff, ctx->buff_size, "%s", "Hello World!\n");

    if (spdk_bdev_is_zoned(ctx->bdev)) {
        hello_reset_zone(ctx);
        return;
    }

    hello_write(ctx);
}

int main(int argc, char **argv) {
    struct spdk_app_opts opts{};
    int rc = 0;
    HelloContext hello_context{};

    spdk_app_opts_init(&opts, sizeof(opts));
    opts.name = "hello_bdev";
    opts.rpc_addr = nullptr;

    if ((rc = spdk_app_parse_args(argc, argv, &opts, "b:", nullptr, hello_bdev_parse_arg,
                                  hello_bdev_usage)) != SPDK_APP_PARSE_ARGS_SUCCESS) {
        std::exit(rc);
    }
    hello_context.bdev_name = g_bdev_name;
    rc = spdk_app_start(&opts, hello_start, &hello_context);
    if (rc) {
        SPDK_ERRLOG("ERROR starting application\n");
    }

    spdk_dma_free(hello_context.buff);
    spdk_app_fini();

    return rc;
}
