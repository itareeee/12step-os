#!/bin/sh -e

# TODO: require sudo on top, but run commands as normal user except for make install, so that most files can be accessed as normal user
#if [[ $UID != 0 ]]; then
#    echo "Please run this script with sudo:"
#    echo "sudo $0 $*"
#    exit 1
#fi

SCRIPT_DIR=$(cd $(dirname $0) && pwd)


####################
# install binutils
####################

if [ ! -e $SCRIPT_DIR/tools/bin/h8300-elf-addr2line ]; then
  cd $SCRIPT_DIR/build
  curl -O http://ftp.gnu.org/gnu/binutils/binutils-2.19.1.tar.bz2
  tar jxf ./binutils-2.19.1.tar.bz2
  
  cd $SCRIPT_DIR/build/binutils-2.19.1
  ./configure --target=h8300-elf --disable-nls --prefix=$SCRIPT_DIR/tools --disable-werror
  make
  
  # TODO: require sudo on top, but run commands as normal user except for make install, so that most files can be accessed as normal user
  sudo make install
fi
