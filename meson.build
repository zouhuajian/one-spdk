project('one-spdk', ['cpp', 'c'],
        version : '1.0.0',
        default_options : ['cpp_std=c++20']
)

# cc = meson.get_compiler('c')

# ✅ 使用 pkg-config 获取系统已安装的 SPDK 和 DPDK 依赖
spdk_deps = [
    dependency('spdk_env_dpdk', required : true),
    dependency('spdk_bdev', required : true),
    dependency('spdk_bdev_malloc', required : true),
    dependency('spdk_bdev_null', required : true),
    dependency('spdk_bdev_nvme', required : true),
    dependency('spdk_bdev_passthru', required : true),
    dependency('spdk_bdev_lvol', required : true),
    dependency('spdk_bdev_raid', required : true),
    dependency('spdk_bdev_error', required : true),
    dependency('spdk_bdev_gpt', required : true),
    dependency('spdk_bdev_split', required : true),
    dependency('spdk_bdev_delay', required : true),
    dependency('spdk_bdev_zone_block', required : true),
    dependency('spdk_blobfs_bdev', required : true),
    dependency('spdk_blobfs', required : true),
    dependency('spdk_blob_bdev', required : true),
    dependency('spdk_lvol', required : true),
    dependency('spdk_blob', required : true),
    dependency('spdk_nvme', required : true),
    dependency('spdk_bdev_aio', required : true),
    dependency('spdk_bdev_ftl', required : true),
    dependency('spdk_ftl', required : true),
    dependency('spdk_bdev_virtio', required : true),
    dependency('spdk_virtio', required : true),
    dependency('spdk_vfio_user', required : true),
    dependency('spdk_accel_error', required : true),
    dependency('spdk_accel_ioat', required : true),
    dependency('spdk_ioat', required : true),
    dependency('spdk_scheduler_dynamic', required : true),
    dependency('spdk_env_dpdk', required : true),
    dependency('spdk_scheduler_dpdk_governor', required : true),
    dependency('spdk_scheduler_gscheduler', required : true),
    dependency('spdk_sock_posix', required : true),
    dependency('spdk_keyring_file', required : true),
    dependency('spdk_keyring_linux', required : true),
    dependency('spdk_fsdev_aio', required : true),
    dependency('spdk_fsdev', required : true),
    dependency('spdk_event', required : true),
    dependency('spdk_event_bdev', required : true),
    dependency('spdk_bdev', required : true),
    dependency('spdk_notify', required : true),
    dependency('spdk_event_accel', required : true),
    dependency('spdk_accel', required : true),
    dependency('spdk_dma', required : true),
    dependency('spdk_event_vmd', required : true),
    dependency('spdk_vmd', required : true),
    dependency('spdk_event_sock', required : true),
    dependency('spdk_sock', required : true),
    dependency('spdk_event_iobuf', required : true),
    dependency('spdk_event_keyring', required : true),
    dependency('spdk_init', required : true),
    dependency('spdk_thread', required : true),
    dependency('spdk_trace', required : true),
    dependency('spdk_keyring', required : true),
    dependency('spdk_rpc', required : true),
    dependency('spdk_jsonrpc', required : true),
    dependency('spdk_json', required : true),
    dependency('spdk_util', required : true),
    dependency('spdk_log', required : true),
]

dpdk_dep = dependency('libdpdk', required : true)
# ✅ 系统依赖
openssl_dep = dependency('openssl', required : true)
uuid_lib_dep = dependency('uuid', required : true)

# ✅ 可执行程序
executable('hello_bdev',
           'hello_bdev.c',
           dependencies : spdk_deps + [dpdk_dep, openssl_dep, uuid_lib_dep],
           link_args : ['-Wl,--no-as-needed'],  # 显式加上链接参数
           install : false,
)
