# zephyr_curing

## Overview
This project implements an esp32 connected to 3 buttons, a SSD1306 OLED display and a motor. My goal was to build my own curing station for 3D prints in resin, and to be able to easily choose the curing time.

The board is made to control A LED strap and a motor both needing 12V to function, and a LED spot working in 230VAC.

The program was made using Zephyr version 4.4 and could be able to run on other architechtures if you modify the app.overlay to fit it, though I didn't try.

## Hardware
Below is a list hardware components.

 * Custom esp-wroom-32D based board (see KiCad project and use an esp32 eval board to follow electrical schematic). Note that the pictures are about Rev A wich is the same with a few errors corrected such as the imprint of the SSR relay inverted, a few GPIO changes for the buttons, and the resistor R3 short-circuited to GND (between the MOSFET).
 * SSD1306, 0.96 inch
 * Capacitors (100nF for motor [not more, it can mess with PWM !] and 10uF as a power bank near the esp32)
 * Resistors (120/10k Ohms)
 * IRLB8721x2 (logic-level nmos) for 12V control and a SSR relay (DA41F) for AC 230V control.
 * 3 buttons
 * 1 diode (1N4007 does the job)
 * 12V power source + 3V3 power source (a LM2596 can generate it with the 12V if needed)

Wire connections as follows

SSD1306 :
 * IO17 <--> SCL
 * IO16 <--> SDA
 * VDD   <--> 3V3 / 5V
 * GND   <--> GND

 Buttons:
 * PIN1 <--> GND
 * PIN2 <--> IO18/19/21

## Software
This project was built with Zephyr 4.4.99 and selects the Nordic esp32_devkitc/esp32/procpu board ().  
Change the "`set(BOARD esp32_devkitc/esp32/procpu)`" in the CMakeFile.txt for other supported boards.

**NOTE:** Make sure that the OLED screen is connected, otherwise the program won't start correctly. Also, this is my first zephyr project so things might not be exactly optimized toward multi-boards compatibility.

## Operation
The screen display the current state of the program and let you either select the time that you want to input, or the remaining time during which the outputs will be powered on.
You can use up and down buttons to increase/decrease time, if you press it for 2s or more, it will automatically increase while you're not releasing it. You can change state by pressing 
In `curing time selection` :
Up button -> increase time, auto-increase if you keep it pressed for 2s or more.
Down button -> decrease time, same as above.
OK button -> validate the time and change state to `curing` (which activates the outputs)

In `curing` :
Up button -> does nothing
Down button -> does nothing
Ok button -> if you press it for 2s or more, stop the outputs and go back to `curing time selection`

## Potential evolutions
The states right now are quite basic, but we could easily add others such as an output selection to select which output we want to activate.
If no screen is available, we could generate an AP and use a captive portal then serve an html page with digital buttons to modify the time and change states
