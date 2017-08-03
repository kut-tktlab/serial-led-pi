#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを点灯させるよ Make an LED strip glitter!
# (c) yoshiaki takata, 2017

from ctypes import cdll	# C言語の関数を呼び出します For calling C functions.
import sys
import time

# LEDテープの接続先GPIO番号 (12,13,18,19のいずれか)
# GPIO number that is connected to an LED strip (12, 13, 18, or 19)
LED_GPIO = 18

# テープ上のLEDの個数 The number of LEDs on the strip.
N_LED = 10

# C言語で書いたライブラリを読み込みます Load a library written in C.
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
# Setting up (if not sudo-ed, this script stops here)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# 25回点灯するよ Change the color 25 times.
for i in range(25):
  for led in range(N_LED):
    # 各LEDがRGBの1色ずつ選ぶよ Each LED selects one of R, G, and B.
    value = 255
    r = value if (led + i) % 3 == 0 else 0
    g = value if (led + i) % 3 == 1 else 0
    b = value if (led + i) % 3 == 2 else 0

    # 色を設定 (まだ送信しない) Set the color (but not transmit yet).
    ledlib.ledSetColor(led, r, g, b)

  # 送信! Transmit the color data!
  ledlib.ledSend()

  # 1秒待ってください Wait for one second.
  time.sleep(1)

# 全部消灯します Turn off all LEDs.
ledlib.ledClearAll()

# 後片付けしてね Please call ledCleanup before exit.
ledlib.ledCleanup()
