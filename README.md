# serial-led-pi

シリアルLEDテープを光らせるサンプルプログラム

```sh
$ make
$ sudo ./ledtape
```

## Files
  - Makefile
  - ledtape.c -- サンプルプログラムのメイン部
  - pwmfifo.c -- PWMのFIFO機能を使うWiringPiもどきライブラリ。[kut-tktlab/play-music-pi](https://github.com/kut-tktlab/play-music-pi/) の pwmfifo.c と同じ。
  - pwmfifo.h -- pwmfifo.cのヘッダファイル
