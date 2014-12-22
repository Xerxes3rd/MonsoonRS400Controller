#ifndef MonsoonRS400_h
#define MonsoonRS400_h

/* ---------------------------------------------
Monsoon RS-400 Microcontroller Pin Configuration
From Component Side, top left is pin 1:
 1 - Vcc (5v)
 2 - GND
 3 - GND
 4 - 10kOhm Resistor to GND
 5 - GND
 6 - N/C
 7 - Pump/Relay Output
 8 - LED2
 9 - LED1

10 - K1 (Btn)
11 - K2 (Btn)
12 - K3 (Btn)
13 - IR Input
14 - Vcc (5v)
15 - Oscillator
16 - Oscillator
17 - W2 (Pot)
18 - W1 (Pot)

J1 From Component Side:
1 - GND
2 - Vcc (5v)
3 - Relay Output

--------------------------------------------- */

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_IIC.h>
#include <EEPROMEx.h>
#include <IRRemote.h>

#define MONSOONRS400_LEDBLINKTIME 350
#define MONSOONRS400_DEBOUNCEDELAY 50

typedef struct {
  int buttonPin;
  bool invert;
} buttonConfig;

typedef struct {
  long lastTime;
  bool lastReading;
  bool currentState;
  bool lastState;
  long lastStateChange;
  bool waitingForRelease;
} buttonState;

enum setupStateEnum { SETCYCLE, SETDURATION, SETCONTRAST, SETLEDBRIGHTNESS, SETLCDBRIGHTNESS };
enum unitStateEnum { POWEROFF, MISTERON, CYCLEON };

class MonsoonRS400
{
  LiquidCrystal_IIC *lcd;
  
  buttonConfig btnConfigs[3];
  buttonState btnStates[3];

  int btnPins[3];

  int ledPin1;
  int ledPin2;
  int misterPin;
  int contrastPin;
  int lcdBacklightPin;

  bool ledState1;// = false;
  bool ledState2;// = false;
  bool misterState;// = false;
  bool lastMisterState;// = false;

  bool inSetup;// = false;

  long lastCountSeconds;// = 0;
  long setupStartTime;// = 0;

  unitStateEnum unitState;// = CYCLEON;
  
  long lastLEDBlink;// = 0;
  
  long lastMisterStart;// = 0;
  long lastCycleStart;// = 0;
  
  int cycleMinutes;// = 720;
  int tempCycleMinutes;// = 720;
  int durationSeconds;// = 12;
  int tempDurationSeconds;// = 12;
  int ledBrightness;// = 30;
  int lcdBrightness;// = 30;
  int contrast;// = 30;
  int defaultCycleIndex;// = 4;
  int defaultDurationIndex;// = 4;
  int defaultContrastIndex;// = 6;
  int defaultLEDBrightnessIndex;// = 3;
  int defaultLCDBrightnessIndex;// = 3;
  
  setupStateEnum setupState;// = SETCYCLE;
  
  int lastIRButtonNumPressed;// = -1;
  long lastIRButtonPressedTime;// = 0;
  bool irButtonHeld;// = false;
  int irButtonBeingHeld;// = -1;
  long irButtonHeldStartTime;// = 0;
  bool irButtonWaitingForRelease;// = false;
  
  int setupButtonHoldTime;
  int setupIRButtonHoldTime;
  
  public:
    MonsoonRS400();
    void setup(LiquidCrystal_IIC *lcdPtr, int ButtonPin1, int ButtonPin2, int ButtonPin3, int LEDPin1, int LEDPin2, int MisterPin, int ContrastPin, int LCDBacklightPin);
    void readEEPROM();
    void updateEEPROM();
    void buttonDown(int buttonNum);
    void buttonUp(int buttonNum);
    void doLoop();
    void setCycleMinutes(int newCycleMinutes);
    void setDurationSeconds(int newDurationSeconds);
    void setContrast(int newContrast, bool doUpdateEEPROM);
    void setLEDBrightness(int newLEDBrightness, bool doUpdateEEPROM);
    void setLCDBrightness(int newLCDBrightness, bool doUpdateEEPROM);
    void processIR(decode_results *results);
    
  private:
    bool debounce(buttonConfig *conf, buttonState *btn);
    bool isHeld(buttonState *btn, int holdTime);
    void writeLEDStates();
    void resetCycle();
    void toggleCycle();
    void startCycle();
    int incrementIndex(int *listPtr, int len, int currentVal, int defaultIndex);
    int decrimentIndex(int *listPtr, int len, int currentVal, int defaultIndex);
    void exitSetup();
    void setLEDs();
    void setMister();
    void updateLCD();
    void printLCDSeconds(long seconds);
    void irButtonDown(int buttonNum);
    void irButtonUp();
    bool isIRButtonHeld(int holdTime);
};

#endif
