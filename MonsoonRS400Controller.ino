// Exo Terra Monsoon RS400 Controller
// Uses the following 3rd-party libraries:
// IRremote: https://github.com/shirriff/Arduino-IRremote
// LiquidCrystal: https://github.com/Xerxes3rd/LiquidCrystal
// EEPROMEx: https://github.com/Xerxes3rd/EEPROMEx

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_IIC.h>
#include <EEPROMEx.h>
#include <IRremote.h>
#include "MonsoonRS400.h"

const int btnPin1 = A0;
const int btnPin2 = A1;
const int btnPin3 = A2;
const int ledPin1 = 5;
const int ledPin2 = 6;
const int misterPin = 7;
const int contrastPin = 10;
const int lcdBacklightPin = 9;
const int irRecvPin = 4;

LiquidCrystal_IIC *lcd = new LiquidCrystal_IIC(IIC_ADDR_UNKNOWN, IIC_BOARD_YWROBOT);
//LiquidCrystal_IIC *lcd = new LiquidCrystal_IIC(IIC_ADDR_UNKNOWN, IIC_BOARD_ADAFRUIT);
MonsoonRS400 *rs400 = new MonsoonRS400();
IRrecv *irrecv = new IRrecv(irRecvPin);

decode_results results;

void setup()
{
  //Serial.begin(115200);
  irrecv->enableIRIn();
  rs400->setup(lcd, btnPin1, btnPin2, btnPin3, ledPin1, ledPin2, misterPin, contrastPin, lcdBacklightPin);
}

void loop()
{
  rs400->doLoop();
  if (irrecv->decode(&results)) {
    rs400->processIR(&results);
    //Serial.println(results.value, HEX);
    irrecv->resume();
  }
}

