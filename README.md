# Binary watch

## BOM
*. JTAG programmer (I've used Seeedstudio XIAO as DAPLINK, see https://wiki.seeedstudio.com/Seeeduino-XIAO-DAPLink/)
*. PCB, files in PCB/ are JLCPCB ready
*. 3D printed or machined case
*. 22mm watch strap
*. At least 3 crocodile pliers
*. 302323 battery (110mAh)
*. Wireless charger

## Getting started with XIAO as DAPLINK
First, connect the programmer. For XIAO, connect
*. XIAO GND to GND
*. XIAO VCC to watch Vin on the microcontroller side
*. XIAO  to Watch SWDIO
*. XIAO  to Watch SWDCLK

Now burn bootloader and program it
*. Start XIAO in bootloader mode (connect usb and restart it twice (see SeeedStudio page)
*. Select Arduino Zero (programming port) board and XIAO port (Don't select XIAO BOARD!)
*. Burn Bootloader
*. Flash board (CRTL + U)

## Print case
Current version should be printed, next will be machinable
Use small layer heights

## PCB
You can provide directly file to JLCPCB
I choosed one side assembly (microcontroler side) to reduce cost
I hand soldered leds, it can be cleaner but it's a fair job. You can train on another pcb because leds are very small. You should use a good quality precision iron, a standard size will make the job too hard and the looks terrible
