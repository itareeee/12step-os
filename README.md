# 12 Step OS

<img src="https://images-na.ssl-images-amazon.com/images/I/51HYm-LUi3L._SX390_BO1,204,203,200_.jpg" height="250"/> <br/>
[12ステップで作る組込みOS自作入門](https://www.amazon.co.jp/12%E3%82%B9%E3%83%86%E3%83%83%E3%83%97%E3%81%A7%E4%BD%9C%E3%82%8B%E7%B5%84%E8%BE%BC%E3%81%BFOS%E8%87%AA%E4%BD%9C%E5%85%A5%E9%96%80-%E5%9D%82%E4%BA%95-%E5%BC%98%E4%BA%AE/dp/4877832394/)


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
