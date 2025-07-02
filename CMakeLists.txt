cmake_minimum_required(VERSION 3.29)
project(one_cpp)

set(CMAKE_CXX_STANDARD 20)

# SPDK 目录
set(SPDK_DIR "~/data/projects/jay/spdk")
set(SPDK_BUILD_DIR "${SPDK_DIR}/build")

# ✅ 包含头文件路径
include_directories(
        ${SPDK_DIR}/include
)

# ✅ 链接库路径
link_directories(
        ${SPDK_DIR}/build/lib
)

# ✅ 生成可执行文件
add_executable(rw_demo rw_demo.cpp)

# ✅ 链接 SPDK 所需库（兼容 SPDK v25.09.0）
target_link_libraries(rw_demo
        spdk
        spdk_env_dpdk
        spdk_bdev
        spdk_bdev_module
        spdk_event
        spdk_thread
        spdk_log
        spdk_util
        spdk_json
        spdk_jsonrpc
        spdk_conf
        spdk_app
        spdk_rpc
        spdk_sock
        spdk_nvme
        spdk_malloc
        spdk_bdev_malloc
        spdk_trace_parser
        pthread
        rt
        dl
        numa
)
