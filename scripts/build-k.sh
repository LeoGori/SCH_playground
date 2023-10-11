#!/bin/sh
# build linux

mkdir -p /staging/initramfs/fs

cd /sources/linux
make x86_64_defconfig

sed -i "s/CONFIG_RANDOMIZE_BASE=y/CONFIG_RANDOMIZE_BASE=n/g" .config
sed -i '$a\CONFIG_FRAME_POINTER=y' .config
sed -i '$a\CONFIG_DEBUG_INFO_REDUCED=n' .config
sed -i '$a\CONFIG_GDB_SCRIPTS=y' .config
sed -i '$a\CONFIG_KALLSYMS_ALL=y' .config
make -j4 bzImage


# build busybox
cd /sources/busybox-1.32.1
make defconfig
LDFLAGS="--static" make -j4 install

