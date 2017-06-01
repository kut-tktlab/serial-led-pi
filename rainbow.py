#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを虹色に点灯させるよ
# (c) kut-tktlab, 2017

from ctypes import cdll, c_int	# C言語の関数を呼び出します
import sys
import time

# LEDテープの接続先GPIO番号 (18 or 19)
LED_GPIO = 18

# テープ上のLEDの個数
N_LED = 10

# frame per second
FPS = 30

# C言語で書いたライブラリを読み込みます
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# 20秒間点灯するよ
for i in range(20 * FPS):
  for led in range(N_LED):
    h = (led * (360 // N_LED) + i * 2) % 360
    s = 255
    b = 128

    # 色を設定 (まだ送信しない)
    ledlib.ledSetColorHSB(c_int(led), c_int(h), c_int(s), c_int(b))

  # 送信!
  ledlib.ledSend()

  # 次のフレームまで待ってください
  time.sleep(1.0 / FPS)

# 全部消灯します
ledlib.ledClearAll()

# 後片付けしてね
ledlib.ledCleanup()
