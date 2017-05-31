# serial-led-pi

シリアルLEDテープを光らせるサンプルプログラム

```sh
$ make
$ sudo ./ledtape
$ sudo ./rainbow
```

## Files
  - Makefile
  - test.c -- サンプルプログラム
  - rainbow.c -- サンプルその2。
  - serialled.c -- シリアルLEDテープを制御するライブラリ。pwmfifo.cを使用。
  - pwmfifo.c -- PWMのFIFO機能を使うWiringPiもどきライブラリ。mailbox.cを使用。
  - mailbox.c -- (c) Broadcom Europe Ltd. メモリのbusアドレスを調べるのに利用。

