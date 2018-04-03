# 12 Step OS

## Links

- [satfyの日記: 2010-12-26 MacでH８開発環境構築 - 12ステップ本を試す](http://d.hatena.ne.jp/satfy/20101226)
- [三等兵: 2回目のC言語で『12ステップで作る組込みOS自作入門 』の通りに組込みOSを作ってみた](http://d.hatena.ne.jp/sandai/20120917/p1)
- http://blog.livedoor.jp/noanoa07/archives/1991674.html

## Setup

```
$ sudo ./setup.sh
```

```
$ cd ./src/01/bootload
$ make
$ sudo H8WRITE_SERDEV=/dev/cu.usbserial-FTYVCLY9 make write
```

## Other
- http://www.ftdichip.com/Drivers/VCP.htm
- brew install minicom
  - needs setup (sudo minicom -s -o)
