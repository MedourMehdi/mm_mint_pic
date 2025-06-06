#!/bin/bash
make clean
/Users/mmedour/switch_env.sh -m68020-60
make -f makefile
rm -rf release
mkdir -p release
cp -a bin/mm_pic.prg release/mm_pic_020.prg
cp -a bin/conf release/conf
cp -a bin/fonts release/fonts
cp -a bin/rsc release/rsc
cp -a bin/ico24 release/ico24

tar --disable-copyfile --no-xattrs --exclude=".*" -cvJf mm_pic_020.tar.xz release