/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (C) 2018 Intel Corporation.
 *   All rights reserved.
 */

#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/bdev_zone.h"
#include <memory>
#include <string>

static std::string g_bdev_name = "Malloc0";

/*
 * We'll use this class to gather housekeeping hello_context to pass between
 * our events and callbacks.
 */
class HelloContext {
public:
    HelloContext(const std::string& name) : bdev_name(name) {}
    ~HelloContext() {
        if (buff) {
            spdk_dma_free(buff);
        }
    }

    // Disable copy operations
    HelloContext(const HelloContext&) = delete;
    HelloContext& operator=(const HelloContext&) = delete;

    struct spdk_bdev *bdev = nullptr;
    struct spdk_bdev_desc *bdev_desc = nullptr;
    struct spdk_io_channel *bdev_io_channel = nullptr;
    char *buff = nullptr;
    uint32_t buff_size = 0;
    std::string bdev_name;
    struct spdk_bdev_io_wait_entry bdev_io_wait;
};

/*
 * Usage function for printing parameters that are specific to this application
 */
static void hello_bdev_usage() {
    printf(" -b <bdev>                 name of the bdev to use\n");
}

/*
 * This function is called to parse the parameters that are specific to this application
 */
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

/*
 * Callback function for read io completion.
 */
static void read_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto hello_context = static_cast<HelloContext*>(cb_arg);

    if (success) {
        SPDK_NOTICELOG("Read string from bdev : %s\n", hello_context->buff);
    } else {
        SPDK_ERRLOG("bdev io read error\n");
    }

    // Complete the bdev io and close the channel
    spdk_bdev_free_io(bdev_io);
    spdk_put_io_channel(hello_context->bdev_io_channel);
    spdk_bdev_close(hello_context->bdev_desc);
    SPDK_NOTICELOG("Stopping app\n");
    spdk_app_stop(success ? 0 : -1);
}

static void hello_read(void *arg) {
    auto hello_context = static_cast<HelloContext*>(arg);

    SPDK_NOTICELOG("Reading io\n");
    int rc = spdk_bdev_read(hello_context->bdev_desc, hello_context->bdev_io_channel,
                           hello_context->buff, 0, hello_context->buff_size, read_complete,
                           hello_context);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        // In case we cannot perform I/O now, queue I/O
        hello_context->bdev_io_wait.bdev = hello_context->bdev;
        hello_context->bdev_io_wait.cb_fn = hello_read;
        hello_context->bdev_io_wait.cb_arg = hello_context;
        spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
                               &hello_context->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while reading from bdev: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
    }
}

/*
 * Callback function for write io completion.
 */
static void write_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto hello_context = static_cast<HelloContext*>(cb_arg);

    // Complete the I/O
    spdk_bdev_free_io(bdev_io);

    if (success) {
        SPDK_NOTICELOG("bdev io write completed successfully\n");
    } else {
        SPDK_ERRLOG("bdev io write error: %d\n", EIO);
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    // Zero the buffer so that we can use it for reading
    memset(hello_context->buff, 0, hello_context->buff_size);

    hello_read(hello_context);
}

static void hello_write(void *arg) {
    auto hello_context = static_cast<HelloContext*>(arg);

    SPDK_NOTICELOG("Writing to the bdev\n");
    int rc = spdk_bdev_write(hello_context->bdev_desc, hello_context->bdev_io_channel,
                            hello_context->buff, 0, hello_context->buff_size, write_complete,
                            hello_context);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        // In case we cannot perform I/O now, queue I/O
        hello_context->bdev_io_wait.bdev = hello_context->bdev;
        hello_context->bdev_io_wait.cb_fn = hello_write;
        hello_context->bdev_io_wait.cb_arg = hello_context;
        spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
                              &hello_context->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while writing to bdev: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
    }
}

static void hello_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev,
                              void *event_ctx) {
    SPDK_NOTICELOG("Unsupported bdev event: type %d\n", type);
}

static void reset_zone_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
    auto hello_context = static_cast<HelloContext*>(cb_arg);

    // Complete the I/O
    spdk_bdev_free_io(bdev_io);

    if (!success) {
        SPDK_ERRLOG("bdev io reset zone error: %d\n", EIO);
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    hello_write(hello_context);
}

static void hello_reset_zone(void *arg) {
    auto hello_context = static_cast<HelloContext*>(arg);

    int rc = spdk_bdev_zone_management(hello_context->bdev_desc, hello_context->bdev_io_channel,
                                     0, SPDK_BDEV_ZONE_RESET, reset_zone_complete, hello_context);

    if (rc == -ENOMEM) {
        SPDK_NOTICELOG("Queueing io\n");
        // In case we cannot perform I/O now, queue I/O
        hello_context->bdev_io_wait.bdev = hello_context->bdev;
        hello_context->bdev_io_wait.cb_fn = hello_reset_zone;
        hello_context->bdev_io_wait.cb_arg = hello_context;
        spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
                              &hello_context->bdev_io_wait);
    } else if (rc) {
        SPDK_ERRLOG("%s error while resetting zone: %d\n", spdk_strerror(-rc), rc);
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
    }
}

/*
 * Our initial event that kicks off everything from main().
 */
static void hello_start(void *arg1) {
    auto hello_context = static_cast<HelloContext*>(arg1);
    uint32_t buf_align;

    SPDK_NOTICELOG("Successfully started the application\n");

    /*
     * There can be many bdevs configured, but this application will only use
     * the one input by the user at runtime.
     *
     * Open the bdev by calling spdk_bdev_open_ext() with its name.
     * The function will return a descriptor
     */
    SPDK_NOTICELOG("Opening the bdev %s\n", hello_context->bdev_name.c_str());
    int rc = spdk_bdev_open_ext(hello_context->bdev_name.c_str(), true, hello_bdev_event_cb, nullptr,
                              &hello_context->bdev_desc);
    if (rc) {
        SPDK_ERRLOG("Could not open bdev: %s\n", hello_context->bdev_name.c_str());
        spdk_app_stop(-1);
        return;
    }

    // A bdev pointer is valid while the bdev is opened.
    hello_context->bdev = spdk_bdev_desc_get_bdev(hello_context->bdev_desc);

    SPDK_NOTICELOG("Opening io channel\n");
    // Open I/O channel
    hello_context->bdev_io_channel = spdk_bdev_get_io_channel(hello_context->bdev_desc);
    if (hello_context->bdev_io_channel == nullptr) {
        SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
        return;
    }

    // Allocate memory for the write buffer.
    // Initialize the write buffer with the string "Hello World!"
    hello_context->buff_size = spdk_bdev_get_block_size(hello_context->bdev) *
                             spdk_bdev_get_write_unit_size(hello_context->bdev);
    buf_align = spdk_bdev_get_buf_align(hello_context->bdev);
    hello_context->buff = static_cast<char*>(spdk_dma_zmalloc(hello_context->buff_size, buf_align, nullptr));
    if (!hello_context->buff) {
        SPDK_ERRLOG("Failed to allocate buffer\n");
        spdk_put_io_channel(hello_context->bdev_io_channel);
        spdk_bdev_close(hello_context->bdev_desc);
        spdk_app_stop(-1);
        return;
    }
    snprintf(hello_context->buff, hello_context->buff_size, "%s", "Hello World!\n");

    if (spdk_bdev_is_zoned(hello_context->bdev)) {
        hello_reset_zone(hello_context);
        // If bdev is zoned, the callback, reset_zone_complete, will call hello_write()
        return;
    }

    hello_write(hello_context);
}

int main(int argc, char **argv) {
    struct spdk_app_opts opts = {};
    int rc = 0;

    // Set default values in opts structure.
    spdk_app_opts_init(&opts, sizeof(opts));
    opts.name = "hello_bdev";
    opts.rpc_addr = nullptr;

    /*
     * Parse built-in SPDK command line parameters as well
     * as our custom one(s).
     */
    if ((rc = spdk_app_parse_args(argc, argv, &opts, "b:", nullptr, hello_bdev_parse_arg,
                                hello_bdev_usage)) != SPDK_APP_PARSE_ARGS_SUCCESS) {
        exit(rc);
    }

    // Create context with automatic memory management
    auto hello_context = std::make_unique<HelloContext>(g_bdev_name);

    /*
     * spdk_app_start() will initialize the SPDK framework, call hello_start(),
     * and then block until spdk_app_stop() is called (or if an initialization
     * error occurs, spdk_app_start() will return with rc even without calling
     * hello_start().
     */
    rc = spdk_app_start(&opts, hello_start, hello_context.get());
    if (rc) {
        SPDK_ERRLOG("ERROR starting application\n");
    }

    // Gracefully close out all of the SPDK subsystems.
    spdk_app_fini();
    return rc;
}