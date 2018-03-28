#!/bin/sh -e

if [[ $UID != 0 ]]; then
    echo "Please run this script with sudo:"
    echo "sudo $0 $*"
    exit 1
fi

SCRIPT_DIR=$(cd $(dirname $0) && pwd)
BUILD_DIR=$SCRIPT_DIR/build
TOOLS_DIR=$SCRIPT_DIR/tools
export PATH=$PATH:$TOOLS_DIR/bin # for gcc to access binutils files under $TOOLS_DIR/bin
set -x

####################
# install binutils
####################

if [ ! -e $TOOLS_DIR/bin/h8300-elf-addr2line ]; then
  cd $BUILD_DIR
  sudo -u $SUDO_USER curl -O http://kozos.jp/books/makeos/binutils-2.19.1.tar.gz
  sudo -u $SUDO_USER tar jxf ./binutils-2.19.1.tar.gz

  cd $BUILD_DIR/binutils-2.19.1
  sudo -u $SUDO_USER ./configure --target=h8300-elf --disable-nls --prefix=$TOOLS_DIR --disable-werror
  sudo -u $SUDO_USER make

  make install
fi

####################
# install gcc
####################
if [ ! -e $SCRIPT_DIR/tools/lib/gcc ]; then
  cd $SCRIPT_DIR/build
  sudo -u $SUDO_USER curl -O http://kozos.jp/books/makeos/gcc-3.4.6.tar.gz
  sudo -u $SUDO_USER tar jxf ./gcc-3.4.6.tar.gz

  cd $SCRIPT_DIR/build/gcc-3.4.6
  sudo -u $SUDO_USER curl -O http://kozos.jp/books/makeos/patch-gcc-3.4.6-x64-h8300.txt
  sudo -u $SUDO_USER patch ./gcc/config/h8300/h8300.c < ./patch-gcc-3.4.6-x64-h8300.txt

  sudo -u $SUDO_USER ./configure --target=h8300-elf --disable-nls --disable-threads --disable-shared --enable-languages=c --disable-werror --prefix=$TOOLS_DIR
  sudo -u $SUDO_USER make

  make install
fi
