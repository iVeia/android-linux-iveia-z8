#!/bin/sh

ROOTFS="./rootfs.cpio"

if [ $1 == "unpack" ]; then
echo unpacking..
echo "making tmp directory" 
mkdir tmp_mnt
sudo sh -c 'cd tmp_mnt/ && cpio -i' < $ROOTFS
rm $ROOTFS
fi


if [ $1 == "pack" ]; then
echo packing..
sh -c 'cd tmp_mnt/ && sudo find . | sudo cpio -H newc -o' > $ROOTFS
echo "removing tmp directory"
rm -r tmp_mnt
echo "new rootfs created at $ROOTFS"
fi

