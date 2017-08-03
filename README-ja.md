# serial-led-pi

<a href="https://www.youtube.com/watch?v=Gf6LSokECh0"><img
src="https://raw.github.com/wiki/kut-tktlab/serial-led-pi/demo1.jpg" width="281"
alt="a link to a demo movie on youtube" /></a>&nbsp;
<a href="https://youtu.be/Gf6LSokECh0?t=12s"><img
src="https://raw.github.com/wiki/kut-tktlab/serial-led-pi/demo2.jpg" width="281"
alt="a link to another demo movie" /></a>

シリアルLEDテープを光らせる簡単なプログラムとライブラリ。<br/>
[Switch Science](https://www.switch-science.com) [SSCI-014007](http://ssci.to/1400) や [Adafruit](https://www.adafruit.com) [NeoPixel](https://www.adafruit.com/category/168) 等のLEDテープ / リングをRaspberry Piで光らせます。

## Quick Start

LEDテープ / リングのData-In端子をGPIO #18に，+5V, GND端子をそれぞれ5V, GND端子に接続し，下記を実行してください。

```sh
$ make                # compile .c files
$ sudo ./sample.py    # run a sample program
$ sudo ./rainbow.py   # run another sample program
```

 - LEDモジュールの個数に応じてPythonプログラム中の `N_LED` の値を変更してください (上記写真 左のテープは10，右のリングは12)。
 - GPIO #18以外の端子を使う場合は，Pythonプログラム中の `LED_GPIO` の値を変更してください。ただし，12, 13, 18, 19 (ハードウェアPWMに接続可能なポート) しか使えません。
 - Raspberry Pi 1やZeroの場合は，pwmfifo.c 中の `PI_VERSION` の値を 1 に変更してから make を実行してください (なお，Raspberry Pi 3 以外の動作確認はしていません)。
 - WS2812Bコントローラ (1ビットの長さ1.25&micro;s, High出力時間 (T0H, T1H) 0.4&micro;s, 0.8&micro;s) の通信仕様に合わせています。ちがう場合は `serialled.c` 中の定数を変更してください。

### For Node.js

Node.jsアドオンのビルドには[node-gyp](https://github.com/nodejs/node-gyp)が必要です。<br/>
node-gypをインストールした後，下記を実行してください。

```sh
$ node-gyp configure build    # build the addon
$ sudo node addon-test.js     # run a testing script
```


## Files
  - Python
    - sample.py -- サンプルプログラム
    - rainbow.py -- サンプルその2
    - beep.py -- もう一方のPWMチャネルで圧電スピーカを鳴らすサンプル
  - C
    - serialled.c -- シリアルLEDテープを制御するライブラリ。pwmfifo.cを使用。
    - pwmfifo.c -- PWMのFIFO機能を使うための[WiringPi](http://wiringpi.com)もどきライブラリ。mailbox.cを使用。
    - mailbox.c -- (c) Broadcom Europe Ltd. メモリを確保してbusアドレスを得るのに利用。
    - Makefile -- 上記をコンパイルする。
  - Node.js
    - addon.cc -- serialled.c 中の関数をNode.jsから使えるようにする
    - binding.gyp -- 上記をビルドする ([node-gyp](https://github.com/nodejs/node-gyp) が必要)。
    - addon-test.js -- 動作テスト用スクリプト

Pythonの [ctypes](https://docs.python.jp/3/library/ctypes.html) ライブラリを使って，serialled.c で定義された関数をPythonスクリプトから呼び出します。

関数の使い方は sample.py, rainbow.py を参照してください。各関数の引数はすべて整数型です。

(Pythonに依存している箇所は特にありません。他の言語から呼び出すのも容易と思います。)

## 内部の仕組み

(研究室メンバー向けの少し詳しい説明は[こちら](https://github.com/kut-tktlab/serial-led-pi/wiki/Pwm)。)

PWMハードウェアに[DMA](https://ja.wikipedia.org/wiki/Direct_Memory_Access)でデータを送り込むことで，PWM信号を送出します (pwmfifo.c)。

- 通信速度が比較的高速
(WS2812Bの場合は 1bit / 1.25 &micro;s = 0.8 Mbps)，かつ数百ビット (= LED数 &times; 24) を途切れずに送信しなければだめ。
- 初期コミットではCPUで送信制御していたが，たまに処理が間に合わないこと (によると思われる間違った色出力) があった。

DMAを使う場合はメモリの物理アドレスを指定する必要があるので，mailbox.c を使って，物理アドレス空間上の領域確保とアドレス取得を行っています。

mailbox.c の使い方も含め，DMAの使い方については下記を参考にしました。

- <https://github.com/metachris/RPIO>
- <https://github.com/hzeller/rpi-gpio-dma-demo>


## License

MIT.

`mailbox.[ch]` are (c) Broadcom Europe Ltd. and taken from
subdirectory `/host_applications/linux/apps/hello_pi/hello_fft` of
<https://github.com/raspberrypi/userland/>.<br/>
See the top of those files for their license.

## Note

[Adafruit](https://www.adafruit.com) から
RaspPi + NeoPixel用の[ライブラリ](https://learn.adafruit.com/neopixels-on-raspberry-pi/software)が提供されているようなので，最初からそれを使えばよかったのでは? と自分でも思いますが…。

単純で，標準のツールとライブラリだけで使えるのがささやかな存在意義かと思います。
