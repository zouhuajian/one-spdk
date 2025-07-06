```shell
# 分配 hugepages
sudo HUGEMEM=1024 $SPDK_DIR/scripts/setup.sh

# export SPDK_DIR=/your/path/to/spdk

rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)

sudo ./hello_bdev -c ../bdev.json
  
  
rm -rf buildDir && meson setup buildDir && ninja -C buildDir 

sudo ./buildDir/hello_bdev -c bdev.json
```