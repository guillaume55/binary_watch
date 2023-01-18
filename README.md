# Binary watch

## How to read time
How to read it (BCD, binary coded digits)
Each digit is separated. The two first digits are for the hours, the two last for minutes
the Bottom digit adds 1, the second 2, the third 4, and the top one 8
For instance if # are on and _ are off

```
_ _ _ _
_ _ # #
_ # _ #
# # _ #

1 3:4 7
```

## Features
* Read time and date
* Set time and date
* Use stopwatch
* Battery level

## How to use it

### Powered on, screen off (Idle)
Use top button to unlock (wake up from sleep) the watch. It will go to sleep after ON_DURATION (default 10s)

### Main screen with menus
* Right button: Set time (and date)
* Top button: Return to time display (menu index 0, useful if you are lost)
* Left button: Change mode, Time(1), Date(2), Stopwatch(3), Remaining battery(4) in this order. Lower led will be on for one second to indicate the index before displaying the mode

### Set time and date
An animation will tell you that you have clicked on setTime
* Right button: go to next digit to modify. Hours are modified at the same time, minutes are digit by digit)
* Top button: no action
* Left button: increase the time or date

A second animation (faster) will tell you that you now have to set the date (DD/MM). When done, press left. Last animation, and main screen will appear. Year should be written manually

### Stopwatch
* Right button: Next menu (will be battery level)
* Top button: No action
* Left button: Pause/Resume

## BOM
* SWD programmer (I use Seeedstudio XIAO as DAPLINK, see https://wiki.seeedstudio.com/Seeeduino-XIAO-DAPLink/)
* PCB, files in PCB/ are JLCPCB ready
* 3D printed or machined case
* 22mm watch strap
* At least 3 crocodile pliers
* 302323 battery (110mAh)
* Wireless charger

## Getting started with XIAO as DAPLINK
First, connect the programmer. For XIAO, connect
* XIAO GND to GND
* XIAO VCC to watch Vin on the microcontroller side
* XIAO 9 to Watch SWDIO
* XIAO 10 to Watch SWDCLK

Install libraries
* Adafruit freetouch (from arduino library manager)
* RtcCounter (By gabriel Notman, from arduino library manager)

Now burn bootloader and program it
* Start XIAO in bootloader mode (connect usb and restart it twice (see SeeedStudio page)
* Select Arduino Zero (programming port) board and XIAO port (Don't select XIAO BOARD!)
* Burn Bootloader
* Flash board (CRTL + U)

## Print case
Current version should be printed, next will be machinable
Use small layer heights

## PCB
You can provide directly file to JLCPCB
I have chosen one side assembly (microcontroler side) to reduce cost
I hand soldered leds, it can be cleaner but it's a fair job. You can train on another PCB because leds are very small. You should use a good quality precision iron, a standard size will make the job too hard and the looks terrible

## Possible improvements
* Don't use arduino framework, there is room to improve consumption by reducing clock speed for example
* Use a more precise oscillator (as the precision is very low) or use the internal oscillator only
