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

# Use SD controller so the PL181 driver can access the disk
DRIVE_IF="sd"

if [ "$MODE" == "gui" ]; then
        qemu-system-arm \
        -M versatilepb \
        -m 128M \
        -semihosting \
        -net nic,model=smc91c111 \
        -net user \
            -drive file=$DISK_IMG,format=raw,if=${DRIVE_IF} \
        -serial stdio \
        -kernel build/kernel.bin
else
    # Console mode (no graphics)
    qemu-system-arm \
        -M versatilepb \
        -m 128M \
        -nographic \
        -semihosting \
        -net nic,model=smc91c111 \
        -net user \
        -drive file=$DISK_IMG,format=raw,if=${DRIVE_IF} \
        -kernel build/kernel.bin
fi