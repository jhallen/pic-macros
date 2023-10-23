# pic-macros

Assembly language macros that make programming PIC microcontrollers more pleasant

The general idea of these macros is to hide the W register and make PIC look
like a two-address machine with mnemonics similar to 6800 / 6502 / PDP-11.

The syntax used is compatible with Microchip's [MPASM](https://ww1.microchip.com/downloads/en/DeviceDoc/30400g.pdf)
and the open source [GPASM](https://gputils.sourceforge.io/).

Simply include "macros.inc" into your assembly source file with:

~~~
	include p10f200.inc
	include macros.inc
~~~

## Features

* The macros hide register banking.  On entry, each macro assumes that the
current bank is bank 0.  If the macro needs to switch banks to access a
register which is located beyond bank 0, it automatically does so.  It then
restores the current bank back to 0.

* The macros are provided for multi-precession arithmetic.
