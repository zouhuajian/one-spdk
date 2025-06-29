project('one-spdk', ['cpp'],
        version : '1.0.0',
        default_options : ['cpp_std=c++20']
)

cpp = meson.get_compiler('cpp')

spdk_root = '/data/projects/jay/spdk'
spdk_lib_dir = spdk_root + '/build/lib'
spdk_include_dir = spdk_root + '/include'

dpdk_lib_dir = spdk_root + '/dpdk/build/lib'
dpdk_include_dir = spdk_root + '/dpdk/build/include'

spdk_includes = include_directories([spdk_include_dir])
dpdk_includes = include_directories([dpdk_include_dir])

# ✅ 系统 DPDK 依赖
dpdk_dep = dependency('libdpdk', required : true)
openssl_dep = dependency('openssl', required : true)
uuid_lib_dep = dependency('uuid', required : true)

# ✅ SPDK 静态库依赖
spdk_libs = [
    cpp.find_library('spdk_accel', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_accel_error', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_accel_ioat', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_aio', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_delay', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_error', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_ftl', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_gpt', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_lvol', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_malloc', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_nvme', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_null', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_passthru', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_raid', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_split', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_bdev_virtio', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_blob', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_blob_bdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_blobfs', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_blobfs_bdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_conf', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_dma', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_env_dpdk', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_env_dpdk_rpc', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_accel', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_bdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_fsdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_iobuf', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_keyring', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_nbd', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_nvmf', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_scsi', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_scheduler', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_sock', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_vhost_blk', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_vhost_scsi', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_event_vmd', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_ftl', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_fsdev', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_fsdev_aio', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_fuse_dispatcher', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_init', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_ioat', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_json', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_jsonrpc', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_keyring', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_keyring_file', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_keyring_linux', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_log', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_nbd', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_nvme', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_nvmf', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_notify', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_rpc', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_scheduler_dpdk_governor', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_scheduler_dynamic', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_scheduler_gscheduler', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_scsi', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_sock', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_sock_posix', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_thread', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_trace', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_trace_parser', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_ut', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_ut_mock', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_vhost', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_vfio_user', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_virtio', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_vmd', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_util', dirs : [spdk_lib_dir]),
    cpp.find_library('spdk_util', dirs : [spdk_lib_dir]),
]

dpdk_libs = [
    cpp.find_library('rte_eal', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_mempool', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_mbuf', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_ring', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_bus_pci', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_pci', dirs : [dpdk_lib_dir], required : true),
    cpp.find_library('rte_kvargs', dirs : [dpdk_lib_dir], required : true, static : true),
]

executable('main',
           'src/main.cpp',
           include_directories : [spdk_includes, dpdk_includes],
           dependencies : spdk_libs + dpdk_libs + [openssl_dep, uuid_lib_dep],
           install : false,
)