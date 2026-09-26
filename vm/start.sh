#!/usr/bin/env bash

set -e

DIR="$(cd "$(dirname "$0")" && pwd)"

qemu-system-x86_64 \
    -enable-kvm \
    -machine q35 \
    -cpu host \
    -smp 2 \
    -m 2048 \
    \
    -drive file="$DIR/alpine.qcow2",format=qcow2 \
    \
    -device edu \
    \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device virtio-net-pci,netdev=net0 \
    \
    -virtfs local,path=/home/cjrevelas/gitRepos/pci_lab/edu-driver,mount_tag=hostshare,security_model=none,id=hostshare
