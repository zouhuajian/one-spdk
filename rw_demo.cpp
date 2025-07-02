#include <iostream>
#include "spdk/stdinc.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/bdev.h"
#include "spdk/string.h"

constexpr const char* BDEV_NAME = "Malloc0";
constexpr size_t BLOCK_SIZE = 4096;

static struct spdk_bdev_desc* g_bdev_desc = nullptr;
static struct spdk_bdev* g_bdev = nullptr;
static struct spdk_io_channel* g_io_channel = nullptr;
static bool g_done = false;

// Forward declarations
void cleanup_and_stop(int exit_code);

void read_complete(struct spdk_bdev_io* bdev_io, bool success, void* ctx) {
    char* buffer = static_cast<char*>(ctx);

    if (success) {
        SPDK_NOTICELOG("[READ SUCCESS] Data: %.16s\n", buffer);
    } else {
        SPDK_ERRLOG("[READ FAILED]\n");
    }

    spdk_free(buffer);
    spdk_bdev_free_io(bdev_io);
    cleanup_and_stop(success ? 0 : -1);
}

void write_complete(struct spdk_bdev_io* bdev_io, bool success, void* ctx) {
    spdk_free(ctx);
    spdk_bdev_free_io(bdev_io);

    if (!success) {
        SPDK_ERRLOG("[WRITE FAILED]\n");
        cleanup_and_stop(-1);
        return;
    }

    SPDK_NOTICELOG("[WRITE SUCCESS]\n");

    // Allocate read buffer and issue read
    char* read_buf = static_cast<char*>(
        spdk_zmalloc(BLOCK_SIZE, 0x1000, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA));
    if (!read_buf) {
        SPDK_ERRLOG("Failed to allocate read buffer\n");
        cleanup_and_stop(-1);
        return;
    }

    int rc = spdk_bdev_read(g_bdev_desc, g_io_channel, read_buf, 0, BLOCK_SIZE, read_complete, read_buf);
    if (rc != 0) {
        SPDK_ERRLOG("spdk_bdev_read() failed: %s\n", spdk_strerror(-rc));
        spdk_free(read_buf);
        cleanup_and_stop(-1);
    }
}

void app_start(void*) {
    int rc = spdk_bdev_open_ext(BDEV_NAME, true, nullptr, nullptr, &g_bdev_desc);
    printf("bdev_desc = %p\n", g_bdev_desc);
    if (rc != 0) {
        SPDK_ERRLOG("Failed to open bdev: %s\n", BDEV_NAME);
        cleanup_and_stop(-1);
        return;
    }

    g_bdev = spdk_bdev_desc_get_bdev(g_bdev_desc);
    g_io_channel = spdk_bdev_get_io_channel(g_bdev_desc);
    if (!g_io_channel) {
        SPDK_ERRLOG("Failed to get I/O channel\n");
        cleanup_and_stop(-1);
        return;
    }

    char* write_buf = static_cast<char*>(
        spdk_zmalloc(BLOCK_SIZE, 0x1000, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA));
    if (!write_buf) {
        SPDK_ERRLOG("Failed to allocate write buffer\n");
        cleanup_and_stop(-1);
        return;
    }

    std::strncpy(write_buf, "Hello SPDK World!", BLOCK_SIZE);

    rc = spdk_bdev_write(g_bdev_desc, g_io_channel, write_buf, 0, BLOCK_SIZE,
                         write_complete, write_buf);
    if (rc != 0) {
        SPDK_ERRLOG("spdk_bdev_write() failed: %s\n", spdk_strerror(-rc));
        spdk_free(write_buf);
        cleanup_and_stop(-1);
    }
}

void cleanup_and_stop(int exit_code) {
    if (g_io_channel) {
        spdk_put_io_channel(g_io_channel);
        g_io_channel = nullptr;
    }

    if (g_bdev_desc) {
        spdk_bdev_close(g_bdev_desc);
        g_bdev_desc = nullptr;
    }

    g_done = true;
    spdk_app_stop(exit_code);
}

int main(int argc, char** argv) {
    spdk_app_opts opts = {};
    spdk_app_opts_init(&opts, sizeof(opts));
    opts.name = "spdk_rw_demo";
    opts.iova_mode = "va";
    unsetenv("SPDK_ARGS");  // 防止外部参数污染

    int rc = spdk_app_start(&opts, app_start, nullptr);
    if (rc != 0) {
        SPDK_ERRLOG("SPDK app start failed: %d\n", rc);
        return rc;
    }

    while (!g_done) {
        spdk_pause();
    }

    spdk_app_fini();
    return 0;
}
