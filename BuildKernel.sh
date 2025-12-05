#!/bin/bash

set -e

make clean
make
clear

# Run QEMU with networking enabled
# -semihosting: allows kernel to exit QEMU
# -net nic: creates network interface with SMC91C111 controller
# -net user: user-mode NAT networking (no root required)
qemu-system-arm \
    -M versatilepb \
    -m 128M \
    -nographic \
    -semihosting \
    -net nic,model=smc91c111 \
    -net user \
    -kernel build/kernel.bin