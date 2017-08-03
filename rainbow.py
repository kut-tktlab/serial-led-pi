#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを虹色に点灯させるよ
# Make an LED-strip glitter like a rainbow.
# (c) yoshiaki takata, 2017

from ctypes import cdll	# C言語の関数を呼び出します For calling C functions.
import sys
import time

# LEDテープの接続先GPIO番号 (12,13,18,19のいずれか)
# GPIO number that is connected to an LED strip (12, 13, 18, or 19)
LED_GPIO = 18

# テープ上のLEDの個数 The number of LEDs on the strip.
N_LED = 10

# frame per second
FPS = 30

# C言語で書いたライブラリを読み込みます Load a library written in C.
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
# Setting up (if not sudo-ed, this script stops here)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# 20秒間点灯するよ Glitter for 20 seconds.
for i in range(20 * FPS):
  for led in range(N_LED):
    h = (led * (360 // N_LED) + i * 2) % 360
    s = 255
    b = 128

    # 色を設定 (まだ送信しない) Set the color (but not transmit yet).
    ledlib.ledSetColorHSB(led, h, s, b)

  # 送信! Transmit the color data!
  ledlib.ledSend()

  # 次のフレームまで待ってください Wait until the next frame.
  time.sleep(1.0 / FPS)

# 全部消灯します Turn off all LEDs.
ledlib.ledClearAll()

# 後片付けしてね Please call ledCleanup before exit.
ledlib.ledCleanup()
