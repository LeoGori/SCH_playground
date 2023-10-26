#!/bin/bash

cd /demos/modules
make prepare
make all
make copy-to-fs
cd /scripts
./build-fs.sh
./start-qemu.sh