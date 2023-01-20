# Binary watch

## How to read time
How to read it (BCD, binary coded digits)
Each digit is separated. The two first digits are for the hours, the two last for minutes
the Bottom digit adds 1, the second 2, the third 4, and the top one 8
For instance if # are on and _ are off

```
//First screen, time (24h format)
_ _ _ _
_ _ # #
_ # _ #
# # _ #

1 3:4 7

//second screen, date (DD/MM)
_ _ _ _
_ _ _ _
_ # _ #
# _ _ #

1 2:0 3 (March 12th)

//third screen, stop watch (indicated led will be on for one second (3rd led, 3rd screen) and the stopwatch will begin counting)
_ _ # _
_ _ _ _
_ _ _ _
_ _ _ _

1 3:4 7

//last screen, Full battery level here
_ _ _ #
_ _ _ #
_ _ _ #
_ _ _ #
```

## Features
* Read time and date (24h format, DD/MM)
* Set time and date
* Use stopwatch
* Battery level

## Four different screens
Navigate between screens with right button
Return to first screen with top button
* Display time (24h format)
* Display date (DD/MM)
* Stopwatch
* See battery level (will return to first screen after few seconds)

## How to use it

### Powered on, screen off (Idle)
Use top button to unlock (wake up from sleep) the watch. It will go to sleep after ON_DURATION (default 10s)
It can be a bit long for the watch to show something as the default pooling interval is 3s.

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

### Battery level
Show battery level as a vertical bar on last column
```
//Full battery  --> low battery
_ _ _ #             _ _ _ _
_ _ _ #             _ _ _ _
_ _ _ #             _ _ _ _
_ _ _ #             _ _ _ #

```

## Power consumption
* 0.44mA in sleep (standby) mode. Wake up sometimes to poll user touch on button (Would need more tools to see average consumption)

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
* RTCZero (from arduino library manager)

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

## Known issue
After programming, the watch sleeps at 7.28mA. No idea why, but the problem disappear after restart

## Possible improvements
* Don't use arduino framework, there is room to improve consumption by reducing clock speed for example
* Use a more precise oscillator (as the precision is very low) or use the internal oscillator only
