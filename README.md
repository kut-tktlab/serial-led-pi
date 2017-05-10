# serial-led-pi

シリアルLEDテープを光らせるサンプルプログラム

```sh
$ make
$ sudo ./ledtape
```

## Files
  - Makefile
  - test.c -- サンプルプログラム
  - serialled.c -- シリアルLEDテープを制御するライブラリ。pwmfifo.cを使用。
  - serialled.h -- ヘッダファイル
  - pwmfifo.c -- PWMのFIFO機能を使うWiringPiもどきライブラリ。[kut-tktlab/play-music-pi](https://github.com/kut-tktlab/play-music-pi/) の pwmfifo.c と同じ。
  - pwmfifo.h -- ヘッダファイル
