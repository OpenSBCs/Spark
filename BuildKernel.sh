#!/bin/bash

set -e

make clean
make
clear

MODE=${1:-"gui"}

# Create disk image if it doesn't exist
DISK_IMG="disk.img"
if [ ! -f "$DISK_IMG" ]; then
    echo "Creating 64MB disk image..."
    qemu-img create -f raw "$DISK_IMG" 64M
fi

if [ "$MODE" == "gui" ]; then
    echo "Starting Spark Kernel (GUI mode)..."
    echo "==================================="
    # GUI mode with graphics
    qemu-system-arm \
        -M versatilepb \
        -m 128M \
        -semihosting \
        -net nic,model=smc91c111 \
        -net user \
        -drive file=$DISK_IMG,format=raw,if=sd \
        -serial stdio \
        -kernel build/kernel.bin
else
    echo "Starting Spark Kernel (Console mode)..."
    echo "========================================"
    # Console mode (no graphics)
    qemu-system-arm \
        -M versatilepb \
        -m 128M \
        -nographic \
        -semihosting \
        -net nic,model=smc91c111 \
        -net user \
        -drive file=$DISK_IMG,format=raw,if=sd \
        -kernel build/kernel.bin
fi