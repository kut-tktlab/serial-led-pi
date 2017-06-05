#!/usr/bin/python
# coding: utf-8

# シリアルLEDテープを点灯させるよ. ブザーも鳴らすよ.
# (c) yoshiaki takata, 2017
#
# ハードウェアPWMの2チャネルの一方をLEDテープ, 他方を
# 圧電スピーカのために使います.
# GPIO 18, 19のペア, または12, 13のペアを使ってください.

from ctypes import cdll	# C言語の関数を呼び出します
import sys
import time

# LEDテープの接続先GPIO番号 (12,13,18,19のいずれか)
LED_GPIO = 18

# テープ上のLEDの個数
N_LED = 10

# スピーカの接続先GPIO番号 (LED_GPIOが18ならこちらは19)
SPK_GPIO = 19

# C言語で書いたライブラリを読み込みます
ledlib = cdll.LoadLibrary("./serialled.so")

# セットアップするよ (sudoしないとここで止まる)
if ledlib.ledSetup(LED_GPIO, N_LED) == -1:
  print("cannot setup serial led.")
  sys.exit(1)

# ブザーのセットアップ
ledlib.pinModePwm(SPK_GPIO)	# PWM_OUTPUT
ledlib.pwmSetModeMS(SPK_GPIO)	# mark:space mode
PWM_CLOCK = 20 * 1000 * 1000	# 20MHz (ledSetup()の中でそう設定される)

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

  # 0.1秒ブザー鳴らします
  # pwmSetRange()に周期, pwmWrite()に周期の半分を与えると,
  # デューティ比50%の波形が出力されます.
  # pwmWrite()に0を与えると, (デューティ比0になって)
  # 音が止まります.
  cycle = PWM_CLOCK // 440	# 440Hz
  ledlib.pwmSetRange(SPK_GPIO, cycle)
  ledlib.pwmWrite   (SPK_GPIO, cycle // 2)
  time.sleep(0.1)
  ledlib.pwmWrite   (SPK_GPIO, 0)

  # 0.9秒待ってください
  time.sleep(0.9)

# 全部消灯します
ledlib.ledClearAll()

# 後片付けしてね
ledlib.ledCleanup()
