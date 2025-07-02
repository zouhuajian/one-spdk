#include <iostream>
#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/string.h"

constexpr const char* BDEV_NAME = "Malloc0";
constexpr size_t BLOCK_SIZE = 4096;

static struct spdk_bdev* g_bdev = nullptr;
static struct spdk_bdev_desc* g_bdev_desc = nullptr;
static struct spdk_io_channel* g_io_channel = nullptr;
static bool g_done = false;

void read_complete(struct spdk_bdev_io* bdev_io, bool success, void* ctx) {
    if (success) {
        std::cout << "[READ SUCCESS]" << std::endl;
        auto* buffer = reinterpret_cast<char*>(ctx);
        std::cout << "Read Data: " << std::string(buffer, 16) << std::endl;
    } else {
        std::cerr << "[READ FAILED]" << std::endl;
    }

    spdk_free(ctx);
    spdk_bdev_free_io(bdev_io);
    g_done = true;
}

void write_complete(struct spdk_bdev_io* bdev_io, bool success, void* ctx) {
    spdk_free(ctx);
    spdk_bdev_free_io(bdev_io);

    if (success) {
        std::cout << "[WRITE SUCCESS]" << std::endl;

        char* read_buf = static_cast<char*>(
            spdk_zmalloc(BLOCK_SIZE, 0x1000, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA));
        spdk_bdev_read(g_bdev_desc, g_io_channel, read_buf, 0, BLOCK_SIZE, read_complete, read_buf);
    } else {
        std::cerr << "[WRITE FAILED]" << std::endl;
        g_done = true;
    }
}

void app_start(void*) {
  // Step 1: 打开 bdev（spdk_bdev_open_ext 会内部查找名字）
  if (spdk_bdev_open_ext(BDEV_NAME, true, nullptr, nullptr, &g_bdev_desc) != 0) {
    std::cerr << "Failed to open bdev: " << BDEV_NAME << std::endl;
    spdk_app_stop(-1);
    return;
  }

  // Step 2: 获取 bdev 对象和 I/O 通道
  g_bdev = spdk_bdev_desc_get_bdev(g_bdev_desc);
  g_io_channel = spdk_bdev_get_io_channel(g_bdev_desc);
  if (!g_io_channel) {
    std::cerr << "Failed to get I/O channel" << std::endl;
    spdk_bdev_close(g_bdev_desc);
    spdk_app_stop(-1);
    return;
  }

  // Step 3: 写入数据
  char* write_buf = static_cast<char*>(
      spdk_zmalloc(BLOCK_SIZE, 0x1000, nullptr, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA));
  if (!write_buf) {
    std::cerr << "Failed to allocate write buffer" << std::endl;
    spdk_put_io_channel(g_io_channel);
    spdk_bdev_close(g_bdev_desc);
    spdk_app_stop(-1);
    return;
  }

  strcpy(write_buf, "Hello SPDK World!");

  int rc = spdk_bdev_write(g_bdev_desc, g_io_channel, write_buf, 0, BLOCK_SIZE,
                           write_complete, write_buf);

  if (rc != 0) {
    std::cerr << "Write failed: " << spdk_strerror(-rc) << std::endl;
    spdk_free(write_buf);
    spdk_put_io_channel(g_io_channel);
    spdk_bdev_close(g_bdev_desc);
    spdk_app_stop(-1);
  }
}


int main(int argc, char** argv) {
    spdk_app_opts opts = {};
    spdk_app_opts_init(&opts, sizeof(opts));
    opts.name = "spdk_rw_demo";
    opts.iova_mode = "va";
    unsetenv("SPDK_ARGS");

    if (spdk_app_start(&opts, app_start, nullptr) != 0) {
        std::cerr << "SPDK app start failed" << std::endl;
        return -1;
    }

    while (!g_done) {
        spdk_pause();
    }

    if (g_io_channel) {
        spdk_put_io_channel(g_io_channel);
    }

    if (g_bdev_desc) {
        spdk_bdev_close(g_bdev_desc);
    }

    spdk_app_fini();
    return 0;
}
