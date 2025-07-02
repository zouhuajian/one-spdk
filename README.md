```shell
# 分配 hugepages
sudo HUGEMEM=1024 $SPDK_DIR/scripts/setup.sh

# export SPDK_DIR=/your/path/to/spdk

mkdir build && cd build
cmake ..
make -j

sudo ./rw_demo -c malloc_bdev.json
```