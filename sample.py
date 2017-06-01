#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを点灯させるよ
# (c) kut-tktlab, 2017

from ctypes import cdll	# C言語の関数を呼び出します
import sys
import time

# LEDテープの接続先GPIO番号 (18 or 19)
LED_GPIO = 18

# テープ上のLEDの個数
N_LED = 10

# C言語で書いたライブラリを読み込みます
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# 25回点灯するよ
for i in range(25):
  for led in range(N_LED):
    # 各LEDがRGBの1色ずつ選ぶよ
    value = 255
    r = value if (led + i) % 3 == 0 else 0
    g = value if (led + i) % 3 == 1 else 0
    b = value if (led + i) % 3 == 2 else 0

    # 色を設定 (まだ送信しない)
    ledlib.ledSetColor(led, r, g, b)

  # 送信!
  ledlib.ledSend()

  # 1秒待ってください
  time.sleep(1)

# 全部消灯します
ledlib.ledClearAll()

# 後片付けしてね
ledlib.ledCleanup()
