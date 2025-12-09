#!/bin/bash

set -e

make clean
make
clear

DISK_IMG="disk.img"
if [ ! -f "$DISK_IMG" ]; then
    echo "Creating 64MB disk image..."
    qemu-img create -f raw "$DISK_IMG" 64M
fi


DRIVE_IF="sd"

echo "Starting Spark Kernel (Console mode)..."
echo "========================================"
    qemu-system-arm \
        -M versatilepb \
        -m 128M \
        -nographic \
        -semihosting \
        -net nic,model=smc91c111 \
        -net user \
        -drive file=$DISK_IMG,format=raw,if=${DRIVE_IF} \
        -kernel build/kernel.bin