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
if [ ! -e $TOOLS_DIR/lib/gcc ]; then
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

####################
# install kz_h8write
####################

if [ ! -e $TOOLS_DIR/h8write ]; then
  cd $SCRIPT_DIR/build
  sudo -u $SUDO_USER curl -O http://mes.osdn.jp/h8/h8write.c
  sudo -u $SUDO_USER mkdir ../tools/h8write
  sudo -u $SUDO_USER gcc -Wall -o ../tools/h8write/h8write h8write.c
fi

####################
# install kz_xmodem
####################

if [ ! -e $TOOLS_DIR/kz_xmodem ]; then
  sudo -u $SUDO_USER mkdir -p $SCRIPT_DIR/build/kz_xmodem
  sudo -u $SUDO_USER mkdir -p $TOOLS_DIR/kz_xmodem

  cd $SCRIPT_DIR/build/kz_xmodem
  sudo -u $SUDO_USER curl -O http://jaist.dl.osdn.jp/kz-xmodem/55725/kz_xmodem-v0.0.2.tar.gz
  sudo -u $SUDO_USER tar jxf ./kz_xmodem-v0.0.2.tar.gz

  cd $SCRIPT_DIR/build/kz_xmodem/src
  sudo -u $SUDO_USER make
  sudo -u $SUDO_USER cp kz_xmodem $TOOLS_DIR/kz_xmodem/
fi
