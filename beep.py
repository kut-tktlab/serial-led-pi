#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを点灯させるよ. ブザーも鳴らすよ.
# Make an LED strip glitter! Also beep a speaker!
# (c) yoshiaki takata, 2017
#
# ハードウェアPWMの2チャネルの一方をLEDテープ, 他方を
# 圧電スピーカのために使います.
# GPIO 18, 19のペア, または12, 13のペアを使ってください.
#
# This script uses the two channels of the hardware PWM of Raspberry Pi.
# One channel for an LED strip and the other for a piezo speaker.
# Please use a pair of GPIO #18 and 19 or a pair of GPIO #12 and 13.

from ctypes import cdll	# C言語の関数を呼び出します For calling C functions.
import sys
import time

# LEDテープの接続先GPIO番号 (12,13,18,19のいずれか)
# GPIO number that is connected to an LED strip (12, 13, 18, or 19)
LED_GPIO = 18

# テープ上のLEDの個数 The number of LEDs on the strip.
N_LED = 10

# スピーカの接続先GPIO番号 (LED_GPIOが18ならこちらは19)
# GPIO number that is connected to a piezo speaker.
# (If LED_GPIO == 18, then SPK_GPIO must be 19.)
SPK_GPIO = 19

# C言語で書いたライブラリを読み込みます Load a library written in C.
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
# Setting up (if not sudo-ed, this script stops here)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# ブザーのセットアップ Setting up the speaker.
ledlib.pinModePwm(SPK_GPIO)	# PWM_OUTPUT
ledlib.pwmSetModeMS(SPK_GPIO)	# mark:space mode
PWM_CLOCK = 20 * 1000 * 1000	# 20MHz (defined by ledSetup())

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

  # 0.1秒ブザー鳴らします Beep for 0.1 seconds.
  # pwmSetRange()に周期, pwmWrite()に周期の半分を与えると,
  # デューティ比50%の波形が出力されます.
  # pwmWrite()に0を与えると, (デューティ比0になって)
  # 音が止まります.
  # Set the cycle time and the half of it to pwmSetRange() and
  # pwmWrite(), respectively. To stop beeping, set 0 to pwmWrite().
  cycle = PWM_CLOCK // 440	# 440Hz
  ledlib.pwmSetRange(SPK_GPIO, cycle)
  ledlib.pwmWrite   (SPK_GPIO, cycle // 2)
  time.sleep(0.1)
  ledlib.pwmWrite   (SPK_GPIO, 0)

  # 0.9秒待ってください Wait for 0.9 second.
  time.sleep(0.9)

# 全部消灯します Turn off all LEDs.
ledlib.ledClearAll()

# 後片付けしてね Please call ledCleanup before exit.
ledlib.ledCleanup()
