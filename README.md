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

