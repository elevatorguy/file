#!/bin/bash
set -e
DISK=""

sudo losetup /dev/loop30 /tmp/file.img
sudo kpartx -a /dev/loop30
#lsblk
sudo mount /dev/mapper/loop30p1 /media/flash/one
sudo mount UUID="$DISK" /media/flash/two
sudo cp /media/flash/one/EFI/BOOT/FILE.EFI /media/flash/two/EFI/BOOT/BOOTX64.EFI
sudo umount /media/flash/one
sudo umount /media/flash/two
sudo kpartx -d /dev/loop30
sudo losetup -d /dev/loop30
