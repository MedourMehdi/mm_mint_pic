#!/bin/bash
make clean
make
mkdir release
cp -a bin/mm_pic.prg release
cp -a bin/conf release/conf
cp -a bin/fonts release/fonts
cp -a bin/rsc release/rsc
cp -a bin/ico24 release/ico24

tar --disable-copyfile --no-xattrs --exclude=".*" -cvJf mm_pic.tar.xz release