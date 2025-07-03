cmake_minimum_required(VERSION 3.20)
project(one_spdk)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SPDK 路径配置
set(SPDK_DIR "/data/projects/jay/spdk")
set(SPDK_INCLUDE_DIR "${SPDK_DIR}/include")
set(SPDK_LIB_DIR "${SPDK_DIR}/build/lib")
# DPDK 库路径（SPDK 内置的 DPDK 编译输出）
set(DPDK_LIB_DIR "${SPDK_DIR}/dpdk/build/lib")

# ✅ 包含头文件
include_directories(${SPDK_INCLUDE_DIR})

# ✅ 自动查找所有 libspdk_*.a
file(GLOB SPDK_STATIC_LIBS "${SPDK_LIB_DIR}/libspdk_*.a")
# GLOB 所有 DPDK 静态库
file(GLOB DPDK_STATIC_LIBS "${DPDK_LIB_DIR}/librte_*.a")


# ✅ 添加可执行文件
add_executable(hello_bdev hello_bdev.cpp)

# ✅ 显式指定库目录（可选）
link_directories(${SPDK_LIB_DIR})

# ✅ 链接 SPDK + 系统库
target_link_libraries(hello_bdev
        -Wl,--start-group
        ${DPDK_STATIC_LIBS}
        ${SPDK_STATIC_LIBS}
        -Wl,--end-group

        pthread
        uuid
        rt
        dl
        numa
        ssl
        crypto
)
