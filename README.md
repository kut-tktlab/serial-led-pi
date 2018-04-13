# serial-led-pi

<a href="https://www.youtube.com/watch?v=Gf6LSokECh0"><img
src="https://raw.github.com/wiki/kut-tktlab/serial-led-pi/demo1.jpg" width="281"
alt="a link to a demo movie on youtube" /></a>&nbsp;
<a href="https://youtu.be/Gf6LSokECh0?t=12s"><img
src="https://raw.github.com/wiki/kut-tktlab/serial-led-pi/demo2.jpg" width="281"
alt="a link to another demo movie" /></a>

A simple library for controlling LED-strips and rings with Raspberry Pi.<br/>
With it, you can make an LED-strip / -ring such as
[Switch Science](https://www.switch-science.com) [SSCI-014007](http://ssci.to/1400) and [Adafruit](https://www.adafruit.com) [NeoPixel](https://www.adafruit.com/category/168) glitter using a Raspberry Pi.

The library is written in C,
and you can use it on Python (using [ctypes](https://docs.python.jp/3/library/ctypes.html))
or on Node.js (as a native add-on).

Note: [Adafruit](https://www.adafruit.com) provides
[a library for RaspPi + NeoPixel](https://learn.adafruit.com/neopixels-on-raspberry-pi/software),
which may be a better solution for the purpose of controlling LED-strips and rings on Raspberry Pi.
A petit raison d'etre of my library is its smallness and simplicity.


## Quick Start

Connect the Data-In port of an LED-strip or ring to the GPIO #18
(in [the GPIO numbering](https://www.raspberrypi.org/documentation/usage/gpio-plus-and-raspi2/), 
not in the physical numbering) pin of a Raspberry Pi
(If you use a Raspberry Pi 1 or Zero, then please see the note below).
Also connect the +5V and GND ports of the strip or ring to the 5V and GND pins of the Raspberry Pi, respectively.

Then, turn on the Raspberry Pi and run the following commands:

```sh
$ make                # compile .c files
$ sudo ./sample.py    # run a sample program
$ sudo ./rainbow.py   # run another sample program
```

Note:

 - For Raspberry Pi 1 and Zero, please change the value of `PI_VERSION` in `pwmfifo.c` into 1 and then run `make` (Note that I have only tested this library on Raspberry Pi 3).
 - Please change the value of `N_LED` in the Python programs according to the number of LEDs on your strip or ring (e.g. 10 LEDs on the strip in the above left-hand picture; 12 LEDs on the ring in the above right-hand picture).
 - To use a GPIO pin other than #18, change the value of `LED_GPIO` in the Python programs. However, you can only use GPIO #12, 13, 18, or 19 (that can be connected to the hardware PWM).
 - Initially this library is configured for WS2812B controller (one bit per 1.25&micro;s; output High for 0.4&micro;s and 0.8&micro;s to represent 0 and 1, respectively). If your LED strip needs different settings, please change parameters defined in `serialled.c`.

The upper limit of `N_LED` (`MAX_N_LED`) is defined as 100 in `serialled.h`.
(A strip with 60 LEDs was used in the movie linked from the below picture.
Note that for such a strip with many LEDs, you may have to provide electricity to the strip not through a Raspberry Pi.
See a [wiki page](https://github.com/kut-tktlab/serial-led-pi/wiki/AcAdapter) for more detail.)

<a href="https://www.youtube.com/watch?v=8v1YTqdLEDs"><img
src="https://raw.github.com/wiki/kut-tktlab/serial-led-pi/demo3.jpg" width="281"
alt="a link to yet another demo movie on youtube" /></a>

### For Node.js

You need [node-gyp](https://github.com/nodejs/node-gyp) to build a Node.js Addon of this library.<br/>
Install node-gyp and then run the following commands:

```sh
$ node-gyp configure build    # build the addon
$ sudo node addon-test.js     # run a testing script
```

## Files
  - Python
    - sample.py -- a sample program
    - rainbow.py -- another sample program
    - beep.py -- yet another sample program that beep a piezo speaker using the other PWM channel
  - C
    - serialled.c -- a library for controlling serial LED strips. It depends on pwmfifo.c.
    - pwmfifo.c -- a [WiringPi](http://wiringpi.com)-like library for using the FIFO of the PWM. It depends on mailbox.c.
    - mailbox.c -- (c) Broadcom Europe Ltd. It defines functions for allocating memory and getting the bus address of it.
    - Makefile -- used for compiling the above C files.
  - Node.js
    - addon.cc -- It makes the functions in serialled.c visible from Node.js.
    - binding.gyp -- a build setting file for addon.cc ([node-gyp](https://github.com/nodejs/node-gyp) is required).
    - addon-test.js -- a testing script of addon.cc.

You can call the functions of this library from your Python script using
[ctypes](https://docs.python.jp/3/library/ctypes.html).
Consult sample.py and rainbow.py for the usage of the library functions.
All functions of this library take int arguments.

(This library does not depend on Python or JavaScript. I think it is not difficult to use the functions on other languages.)

## Internals

([More details in Japanese](https://github.com/kut-tktlab/serial-led-pi/wiki/Pwm).)

It transmits PWM signals to an LED-strip or ring by providing data to the PWM hardware
using [DMA](https://en.wikipedia.org/wiki/Direct_Memory_Access) (in pwmfifo.c).

- Relatively high-speed transmission is required
(for WS2812B, 1bit / 1.25 &micro;s = 0.8 Mbps). Moreover, we have to transmit several handreds of bits (= N_LED &times; 24) without intermission.
- So, providing data to the PWM by CPU (earlier commits of this library were doing so) was not a good idea; it sometimes failed to be in time.

To use DMA,
we have to know the *physical* address of memory.
This library uses mailbox.c to allocate memory in the physical address space and to obtain the address of that memory.

References for the usage of mailbox.c and the DMA controller of Raspberry Pi:

- <https://github.com/metachris/RPIO>
- <https://github.com/hzeller/rpi-gpio-dma-demo>


## License

MIT.

`mailbox.[ch]` are (c) Broadcom Europe Ltd. and taken from
subdirectory `/host_applications/linux/apps/hello_pi/hello_fft` of
<https://github.com/raspberrypi/userland/>.<br/>
See the top of those files for their license.
