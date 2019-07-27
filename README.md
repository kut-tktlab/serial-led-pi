# serial-led-pi

シリアルLEDテープを光らせるサンプルプログラム
(ベアメタル版)

## 使い方

1. `make` を実行。-- `rainbow.img` と `raindrops.img` が作られる。
2. Raspbianをインストール済みのmicroSDカードにコピー。
    - `rainbow.img` か `raindrops.img` のどちらかを，microSDカードの
      boot パーティションに `kernel7.img` という名前でコピー。<br />
      例 `$ cp rainbow.img /Volumes/boot/kernel7.img`
    - (元々ある `kernel7.img` を別のファイル名に変えて保存しておけば，
      Raspbianを復活させるのが容易。)
    - (Raspbianをインストールする代わりに，microSDカードの
      FAT32フォーマットの第1パーティションに
      `bootcode.bin`, `start.elf`, `kernel7.img`
      の3ファイルを置くだけでもよい。)
3. microSDカードをejectし，Rapberry Pi 3 に挿して，電源を入れる。
    - 10個を超えるLEDを駆動するときは，
      Raspberry Piとは別に電源を供給した方がよい。
      GNDの接続を忘れないように。
    - 起動に10秒くらい掛かる場合がある。


## Files
  - Makefile
  - test.c -- サンプルプログラム
  - rainbow.c -- 60個のLEDを虹色に光らせる。
  - raindrops.c -- 10個のLED × 4本 を雨だれ風に光らせる。
  - boot.s -- ベアメタル用スタートアップルーチン
  - serialled.c -- シリアルLEDテープを制御するライブラリ。pwmfifo.cを使用。
  - pwmfifo.c -- PWMのFIFO機能を使うWiringPiもどきライブラリ。[kut-tktlab/play-music-pi](https://github.com/kut-tktlab/play-music-pi/) の pwmfifo.c と同じ。
  - sin.c -- 整数演算のみ使った sin, cos, rand の実装
