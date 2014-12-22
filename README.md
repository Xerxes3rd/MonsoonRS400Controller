MonsoonRS400Controller
======================

Replacement Arduino controller code for the Exo Terra Monsoon RS400.

The PIC that ships with the Exo Terra Monsoon RS400 doesn't properly smooth the readings from the potentiometers.
This sketch is for an Arduino that replaces the PIC and also supports a 16x2 LCD screen.

Dependencies
============

IRremote: https://github.com/shirriff/Arduino-IRremote

LiquidCrystal: https://github.com/Xerxes3rd/LiquidCrystal
  (requires that you rename the LiquidCrystal library that ships with the Arduino IDE)

EEPROMEx: https://github.com/Xerxes3rd/EEPROMEx

Normal Usage
============

The three push buttons operate the unit the same as before: "on" will turn the mister on (indefinitely), "0" will turn the mister off (indefinitely), and "cycle" will cause the unit to operate in its programmed misting cycle.

Setup Mode
==========

To configure the unit, press and hold the "0" button for 3 seconds.  When in configuration mode, both LEDs will flash.  The current value of the displayed setting can be decrimented by pressing the "on" button, and incremented by pressing the "cycle" button.  Pressing the "0" button will change to the next configurable setting.  To exit from the setup mode, either press and hold the "0" button for 3 seconds, or don't press any buttons for 10 seconds.
