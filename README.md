# 12 Step OS

## Initial Setup

- Install binutils/gcc/h8write

  ```
  $ sudo ./setup.sh
  ```

- Install USB-Serial Driver 
  - http://www.ftdichip.com/Drivers/VCP.htm

- Install minicom
  ```
  $ brew install minicom
  $ sudo minicom -s -o    # setup
  ```

- Install lrzsz (for xmodem)
  ```
  $ brew install lrzsz
  ```


## Build Src & Write to H8/3069F

```
$ cd ./src/bootload
$ make

# you could check with "ls -l /dev/cu*"
$ sudo H8WRITE_SERDEV=/dev/cu.usbserial-FTYVCLY9 make write
```

## Other


## Links

- [satfyの日記: 2010-12-26 MacでH８開発環境構築 - 12ステップ本を試す](http://d.hatena.ne.jp/satfy/20101226)
- [三等兵: 2回目のC言語で『12ステップで作る組込みOS自作入門 』の通りに組込みOSを作ってみた](http://d.hatena.ne.jp/sandai/20120917/p1)
- http://blog.livedoor.jp/noanoa07/archives/1991674.html
