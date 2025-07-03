```shell
# 分配 hugepages
sudo HUGEMEM=1024 $SPDK_DIR/scripts/setup.sh

# export SPDK_DIR=/your/path/to/spdk

rm -rf build && mkdir build && cd build && cmake .. && make -j

sudo ./hello_bdev -c malloc_bdev.json

sudo ./hello_bdev -c malloc_bdev.json \
  --log-level=spdk:6 \
  --log-level=lib.bdev:6 \
  --log-level=lib.event:6 \
  --log-level=lib.thread:6 \
  --log-level=user1:6sudo ./rw_demo -c ../malloc_bdev.json -L user1
```