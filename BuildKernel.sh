#!/bin/bash

set -e

make clean
make
clear

MODE=${1:-"gui"}

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
        -kernel build/kernel.bin
fi