project('one-spdk', ['cpp', 'c'],
        version : '1.0.0',
        default_options : ['cpp_std=c++20']
)

# cc = meson.get_compiler('c')

# ✅ 使用 pkg-config 获取系统已安装的 SPDK 和 DPDK 依赖
spdk_deps = [
    dependency('spdk_env_dpdk', required : true),
    dependency('spdk_bdev', required : true),
    dependency('spdk_bdev_nvme', required : true),
    dependency('spdk_blobfs', required : true),
    dependency('spdk_jsonrpc', required : true),
    dependency('spdk_nvme', required : true),
    dependency('spdk_event', required : true),
    dependency('spdk_rpc', required : true),
    dependency('spdk_thread', required : true),
    dependency('spdk_util', required : true),
]

dpdk_dep = dependency('libdpdk', required : true)

# ✅ 系统依赖
openssl_dep = dependency('openssl', required : true)
uuid_lib_dep = dependency('uuid', required : true)


# ✅ 可执行程序
executable('hello_bdev',
           'hello_bdev.c',
           dependencies : spdk_deps + dpdk_dep + [openssl_dep, uuid_lib_dep],
           install : false,
)
