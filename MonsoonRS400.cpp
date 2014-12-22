#include "MonsoonRS400.h"

static int cycles[] = {1, 10, 30, 60, 120, 240, 480, 720, 960, 1200, 1440};
static int durations[] = {2, 4, 6, 8, 12, 16, 18, 20, 30, 45, 60, 90, 120};
static int contrasts[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70};
static int ledBrightnesses[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

typedef enum upDownEnum { NOCHANGE, INCREMENT, DECREMENT };

#define IRKEYUPTIME 100

MonsoonRS400::MonsoonRS400()
{ 
  ledState1 = false;
  ledState2 = false;
  misterState = false;
  lastMisterState = false;

  inSetup = false;

  lastCountSeconds = 0;
  setupStartTime = 0;

  unitState = CYCLEON;
  
  lastLEDBlink = 0;
  lastMisterStart = 0;
  lastCycleStart = 0;
  
  cycleMinutes = 720;
  tempCycleMinutes = 720;
  durationSeconds = 12;
  tempDurationSeconds = 12;
  contrast = 30;
  ledBrightness = 30;
  lcdBrightness = 30;
  defaultCycleIndex = 4;
  defaultDurationIndex = 4;
  defaultContrastIndex = 6;
  defaultLEDBrightnessIndex = 3;
  defaultLCDBrightnessIndex = 3;
  
  lastIRButtonNumPressed = -1;
  lastIRButtonPressedTime = 0;
  irButtonHeld = false;
  irButtonBeingHeld = -1;
  irButtonHeldStartTime = 0;
  irButtonWaitingForRelease = false;

  setupButtonHoldTime = 2000;
  setupIRButtonHoldTime = 1000;
  
  setupState = SETCYCLE;
}

void MonsoonRS400::setup(LiquidCrystal_IIC *lcdPtr, int ButtonPin1, int ButtonPin2, int ButtonPin3, int LEDPin1, int LEDPin2, int MisterPin, int ContrastPin, int LCDBacklightPin)
{
  lcd = lcdPtr;
  memset(&btnConfigs, 0, sizeof(btnConfigs));
  memset(&btnStates, 0, sizeof(btnStates));
  
  btnConfigs[0].buttonPin = ButtonPin1;
  btnConfigs[1].buttonPin = ButtonPin2;
  btnConfigs[2].buttonPin = ButtonPin3;
  
  ledPin1 = LEDPin1;
  ledPin2 = LEDPin2;
  misterPin = MisterPin;
  contrastPin = ContrastPin;
  lcdBacklightPin = LCDBacklightPin;
  
  for (int i = 0; i < 3; i++)
  {
    btnConfigs[i].invert = true;
    pinMode(btnConfigs[i].buttonPin, INPUT);
    digitalWrite(btnConfigs[i].buttonPin, HIGH);
  }
  
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(contrastPin, OUTPUT);
  pinMode(lcdBacklightPin, OUTPUT);
  pinMode(misterPin, OUTPUT);

  readEEPROM();
  
  lcd->begin(16, 2);
  lcd->clear();
  lcd->home();

  writeLEDStates();
  updateLCD();
  resetCycle();
  startCycle();
}

void MonsoonRS400::doLoop()
{
  bool forceUpdateLCD = false;
  
  for (int i = 0; i < 3; i++)
  {
    debounce(&btnConfigs[i], &btnStates[i]);
    if (btnStates[i].currentState != btnStates[i].lastState)
    {
      if (btnStates[i].currentState)
      {
        buttonDown(i);
      }
      else
      {
        buttonUp(i);
      }
    }
  }
  
  if (isHeld(&btnStates[0], setupButtonHoldTime) || isIRButtonHeld(setupIRButtonHoldTime))
  {
    inSetup = !inSetup;
    
    if (inSetup)
    {
      tempCycleMinutes = cycleMinutes;
      tempDurationSeconds = durationSeconds;
      ledState1 = false;
      ledState2 = false;
      setupStartTime = millis();
    }
    else
    {
      exitSetup();
    }    
    
    updateLCD();
  }
  
  if (inSetup)
  {
    if ((millis() - setupStartTime) > 10000)
    {
      exitSetup();
      updateLCD();
    }
  }
  
  if (!inSetup)
  {
    setLEDs();
    
    if (unitState == CYCLEON)
    {
      long currentMisterCountSeconds = (millis() - lastMisterStart) / 1000;
      long currentCycleCountSeconds = (millis() - lastCycleStart) / 1000;
      long cycleSeconds = (long)cycleMinutes * 60;
      long currentCountSeconds = 0;
    
      if (currentCycleCountSeconds >= cycleSeconds)
      {
        misterState = true;
        lastCycleStart = millis();
        forceUpdateLCD = true;
        currentCountSeconds = durationSeconds;
      }
      else if (misterState && (currentMisterCountSeconds > durationSeconds))
      {
        misterState = false;
        forceUpdateLCD = true;
      }
      else if (misterState)
      {
        currentCountSeconds = currentMisterCountSeconds;
      }
      else if (!misterState)
      {
        currentCountSeconds = currentCycleCountSeconds;
      }
      
      if (currentCountSeconds != lastCountSeconds)
      {
        updateLCD();
        lastCountSeconds = currentCountSeconds;
        forceUpdateLCD = false;
      }
    }
  }
  else
  {
    if ((millis() - lastLEDBlink) > MONSOONRS400_LEDBLINKTIME)
    {
      ledState1 = !ledState1;
      ledState2 = !ledState2;
      lastLEDBlink = millis();
    }
  }

  setMister();
  writeLEDStates();
}

bool MonsoonRS400::debounce(buttonConfig *conf, buttonState *btn)
{
  int reading = digitalRead(conf->buttonPin);
  
  if (conf->invert)
  {
    reading = !reading;
  }

  if (reading != btn->lastReading) {
    // reset the debouncing timer
    btn->lastTime = millis();
  } 
  
  if ((millis() - btn->lastTime) > MONSOONRS400_DEBOUNCEDELAY) {
    
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    btn->lastState = btn->currentState;
    
    if (reading != btn->currentState) {
      btn->currentState = reading;
      btn->lastStateChange = millis();
    }
  }
  
  btn->lastReading = reading;
  
  return btn->currentState;
}

bool MonsoonRS400::isHeld(buttonState *btn, int holdTime)
{
  bool held = false;
  
  if (((millis() - btn->lastStateChange) > holdTime) && 
      btn->currentState && 
      !btn->waitingForRelease)
  {
    held = true;
    btn->waitingForRelease = true;
  }
  
  if (!btn->currentState)
  {
    btn->waitingForRelease = false;
  }
  
  return held;
}

void MonsoonRS400::buttonDown(int buttonNum)
{
  if (inSetup)
  {
    bool found = false;
    int i = 0;
    int len = 0;
    int defaultIndex = 0;
    int currentVal = 0;
    int *listPtr = NULL;
    setupStartTime = millis();
    upDownEnum incrementOrDecrement = NOCHANGE;
    
    switch(buttonNum)
    {
      case 0:
        switch (setupState)
        {
          case SETCYCLE:
            setupState = SETDURATION;
            break;
          case SETDURATION:
            setupState = SETCONTRAST;
            break;
          case SETCONTRAST:
            setupState = SETLEDBRIGHTNESS;
            break;
          case SETLEDBRIGHTNESS:
            setupState = SETLCDBRIGHTNESS;
            break;
          case SETLCDBRIGHTNESS:
            setupState = SETCYCLE;
            break;
        }
        break;
      case 1:
        incrementOrDecrement = DECREMENT;
        break;
      case 2:
        incrementOrDecrement = INCREMENT;
        break;
    }
    
    if (incrementOrDecrement != NOCHANGE)
    {
      switch (setupState)
      {
        case SETCYCLE:
          len = sizeof(cycles)/sizeof(int);
          defaultIndex = defaultCycleIndex;
          currentVal = tempCycleMinutes;
          listPtr = (int *)&cycles;
          
          if (incrementOrDecrement == INCREMENT)
          {
            i = incrementIndex(listPtr, len, currentVal, defaultIndex);
          }
          else if (incrementOrDecrement == DECREMENT)
          {
            i = decrimentIndex(listPtr, len, currentVal, defaultIndex);
          }
          
          tempCycleMinutes = cycles[i];
          break;
        case SETDURATION:
          len = sizeof(durations)/sizeof(int);
          defaultIndex = defaultDurationIndex;
          currentVal = tempDurationSeconds;
          listPtr = (int *)&durations;
          
          if (incrementOrDecrement == INCREMENT)
          {
            i = incrementIndex(listPtr, len, currentVal, defaultIndex);
          }
          else if (incrementOrDecrement == DECREMENT)
          {
            i = decrimentIndex(listPtr, len, currentVal, defaultIndex);
          }
          
          tempDurationSeconds = durations[i];            
          break;
        case SETCONTRAST:
          len = sizeof(contrasts)/sizeof(int);
          defaultIndex = defaultContrastIndex;
          currentVal = contrast;
          listPtr = (int *)&contrasts;
          
          if (incrementOrDecrement == INCREMENT)
          {
            i = incrementIndex(listPtr, len, currentVal, defaultIndex);
          }
          else if (incrementOrDecrement == DECREMENT)
          {
            i = decrimentIndex(listPtr, len, currentVal, defaultIndex);
          }
          
          setContrast(contrasts[i], false);
          break;
        case SETLEDBRIGHTNESS:
          len = sizeof(ledBrightnesses)/sizeof(int);
          defaultIndex = defaultLEDBrightnessIndex;
          currentVal = ledBrightness;
          listPtr = (int *)&ledBrightnesses;
          
          if (incrementOrDecrement == INCREMENT)
          {
            i = incrementIndex(listPtr, len, currentVal, defaultIndex);
          }
          else if (incrementOrDecrement == DECREMENT)
          {
            i = decrimentIndex(listPtr, len, currentVal, defaultIndex);
          }
          
          setLEDBrightness(ledBrightnesses[i], false);
          break;
        case SETLCDBRIGHTNESS:
          len = sizeof(ledBrightnesses)/sizeof(int);
          defaultIndex = defaultLCDBrightnessIndex;
          currentVal = lcdBrightness;
          listPtr = (int *)&ledBrightnesses;
          
          if (incrementOrDecrement == INCREMENT)
          {
            i = incrementIndex(listPtr, len, currentVal, defaultIndex);
          }
          else if (incrementOrDecrement == DECREMENT)
          {
            i = decrimentIndex(listPtr, len, currentVal, defaultIndex);
          }
          
          setLCDBrightness(ledBrightnesses[i], false);
          break;
      }
    }
  }
  else
  {
    switch(buttonNum)
    {
      case 0:
        unitState = POWEROFF;
        break;
      case 1:
        unitState = MISTERON;
        break;
      case 2:
        if (unitState != CYCLEON)
        {
          resetCycle();
        }
        unitState = CYCLEON;
        toggleCycle();
        break;
    }
    
    switch (unitState)
    {
      case POWEROFF:
        misterState = false;
        break;
      case MISTERON:
        misterState = true;
        break;
      case CYCLEON:
        break;
    }
  }
  
  updateLCD();
}

void MonsoonRS400::buttonUp(int buttonNum)
{
}

void MonsoonRS400::exitSetup()
{
  cycleMinutes = tempCycleMinutes;
  durationSeconds = tempDurationSeconds;
  setContrast(contrast, true);
  setLEDBrightness(ledBrightness, true);
  setLCDBrightness(lcdBrightness, true);
  updateEEPROM();
  inSetup = false;
}

void MonsoonRS400::writeLEDStates()
{
  int translatedBrightness = 255 - (ledBrightness * 0.5);
  ledState1 ? analogWrite(ledPin1, translatedBrightness) : digitalWrite(ledPin1, true);
  ledState2 ? analogWrite(ledPin2, translatedBrightness) : digitalWrite(ledPin2, true);
}

void MonsoonRS400::readEEPROM()
{
  cycleMinutes = EEPROM.readInt(0);
  durationSeconds = EEPROM.readInt(2);
  contrast = EEPROM.readInt(4);
  ledBrightness = EEPROM.readInt(6);
  lcdBrightness = EEPROM.readInt(8);
  
  if (cycleMinutes < 0)
  {
    cycleMinutes = cycles[defaultCycleIndex];
  }
  
  if (durationSeconds < 0)
  {
    durationSeconds = durations[defaultDurationIndex];
  }
  
  if (contrast < 0)
  {
    contrast = contrasts[defaultContrastIndex];
  }
  
  if (ledBrightness < 0)
  {
    ledBrightness = ledBrightnesses[defaultLEDBrightnessIndex];
  }
  
  if (lcdBrightness < 0)
  {
    lcdBrightness = ledBrightnesses[defaultLCDBrightnessIndex];
  }
  
  setContrast(contrast, false);
  setLEDBrightness(ledBrightness, false);
  setLCDBrightness(lcdBrightness, false);
  
  updateEEPROM();
}

void MonsoonRS400::updateEEPROM()
{
  EEPROM.updateInt(0, cycleMinutes);
  EEPROM.updateInt(2, durationSeconds);
  EEPROM.updateInt(4, contrast);
  EEPROM.updateInt(6, ledBrightness);
  EEPROM.updateInt(8, lcdBrightness);
}

void MonsoonRS400::setCycleMinutes(int newCycleMinutes)
{
  cycleMinutes = newCycleMinutes;
  updateEEPROM();
}

void MonsoonRS400::setDurationSeconds(int newDurationSeconds)
{
  durationSeconds = newDurationSeconds;
  updateEEPROM();
}

void MonsoonRS400::setContrast(int newContrast, bool doUpdateEEPROM)
{
  contrast = newContrast;
  analogWrite(contrastPin, contrast);
  
  if (doUpdateEEPROM)
  {
    updateEEPROM();
  }
}

void MonsoonRS400::setLEDBrightness(int newLEDBrightness, bool doUpdateEEPROM)
{
  ledBrightness = newLEDBrightness;
  
  if (doUpdateEEPROM)
  {
    updateEEPROM();
  }
}

void MonsoonRS400::setLCDBrightness(int newLCDBrightness, bool doUpdateEEPROM)
{
  lcdBrightness = newLCDBrightness;
  analogWrite(lcdBacklightPin, map(newLCDBrightness, 0, 100, 0, 255));
  
  if (doUpdateEEPROM)
  {
    updateEEPROM();
  }
}

void MonsoonRS400::resetCycle()
{
  misterState = false;
  setMister();
  lastCycleStart = millis();
}

void MonsoonRS400::toggleCycle()
{
  misterState = !misterState;
  setMister();
}

void MonsoonRS400::startCycle()
{
  misterState = true;
  setMister();
}

int MonsoonRS400::incrementIndex(int *listPtr, int len, int currentVal, int defaultIndex)
{
  bool found = false;
  int i = 0;
  for (i = 0; i < len; i++)
  {
    if (currentVal == *listPtr)
    {
      found = true;
      break;
    }
    
    listPtr++;
  }
  
  if (!found)
  {
    i = defaultIndex;
  }
  else
  {
    i++;
    
    if (i >= len)
    {
      i = 0;
    }
  }
  
  return i;
}

int MonsoonRS400::decrimentIndex(int *listPtr, int len, int currentVal, int defaultIndex)
{
  bool found = false;
  int i = 0;
  for (i = 0; i < len; i++)
  {
    if (currentVal == *listPtr)
    {
      found = true;
      break;
    }
    
    listPtr++;
  }
  
  if (!found)
  {
    i = defaultIndex;
  }
  else
  {
    i--;
    
    if (i < 0)
    {
      i = len - 1;
    }
  }
  
  return i;
}

void MonsoonRS400::setLEDs()
{
  switch (unitState)
  {
    case POWEROFF:
      ledState1 = false;
      ledState2 = false;
      break;
    case MISTERON:
      ledState1 = true;
      ledState2 = false;
      break;
    case CYCLEON:
      ledState1 = false;
      ledState2 = true;
  }
}

void MonsoonRS400::setMister()
{
  if (misterState != lastMisterState)
  {
    digitalWrite(misterPin, misterState);
    lastMisterState = misterState;
    
    if (misterState)
    {
      lastMisterStart = millis();
    }
  }
}

void MonsoonRS400::updateLCD()
{
  if (inSetup)
  {
    lcd->clear();
    lcd->setCursor(0, 0);
    switch (setupState)
    {
      case SETCYCLE:
        lcd->print("Set Cycle:");
        lcd->setCursor(0, 1);
        if (tempCycleMinutes < 60)
        {
          lcd->print(tempCycleMinutes);
          lcd->print("m");
        }
        else
        {
          lcd->print(tempCycleMinutes / 60);
          lcd->print("h");
        }
        break;
      case SETDURATION:
        lcd->print("Set Duration:");
        lcd->setCursor(0, 1);
        lcd->print(tempDurationSeconds);
        lcd->print("s");
        break;
      case SETCONTRAST:
        lcd->print("Set Contrast:");
        lcd->setCursor(0, 1);
        lcd->print(contrast);
        break;
      case SETLEDBRIGHTNESS:
        lcd->print("Set LED Bright:");
        lcd->setCursor(0, 1);
        lcd->print(ledBrightness);
        break;
      case SETLCDBRIGHTNESS:
        lcd->print("Set LCD Bright:");
        lcd->setCursor(0, 1);
        lcd->print(lcdBrightness);
        break;
    }
  }
  else
  {
    lcd->clear();
    lcd->setCursor(0, 0);

    lcd->print("C: ");
    if (cycleMinutes < 60)
    {
      lcd->print(cycleMinutes);
      lcd->print("m");
    }
    else
    {
      lcd->print(cycleMinutes / 60);
      lcd->print("h");
    }

    lcd->print(" D: ");
    lcd->print(durationSeconds);
    lcd->print("s");
    lcd->setCursor(0, 1);
    
    long secondsSinceMisterStart = 0;
    long secondsSinceCycleStart = 0;
    long countDown = 0;
    
    switch (unitState)
    {
      case POWEROFF:
        lcd->print("POWER OFF");
        break;
      case MISTERON:
        lcd->print("MISTER ON");
        break;
      case CYCLEON:
        secondsSinceMisterStart = (millis() - lastMisterStart) / 1000;
        secondsSinceCycleStart = (millis() - lastCycleStart) / 1000;
        if (misterState)
        {
          lcd->print("MIST for ");
          countDown = durationSeconds - secondsSinceMisterStart;
        }
        else
        {
          lcd->print("Next:");
          countDown = ((long)cycleMinutes * 60) - secondsSinceCycleStart;
        }
        printLCDSeconds(countDown);
        break;
    }
  }
}

void MonsoonRS400::printLCDSeconds(long seconds)
{
  int hourDisp = 0;
  int minsDisp = seconds / 60;
  int secDisp = seconds % 60;
  bool hourShown = false;
  
  if (minsDisp > 60)
  {
    hourDisp = minsDisp / 60;
    minsDisp = minsDisp % 60;
  }

  if (hourDisp > 0)
  {
    lcd->print(hourDisp);
    lcd->print("h ");
    hourShown = true;
  }
  
  if ((minsDisp > 0) || hourShown)
  {
    lcd->print(minsDisp);
    lcd->print("m ");
  }
  
  lcd->print(secDisp);
  lcd->print("s");
}

void MonsoonRS400::processIR(decode_results *results)
{
  int buttonNum = -1;
  
  switch (results->value)
  {
    case 0x371A3C86:
      buttonNum = 0;
      break;
    case 0xE0984BB6:
      buttonNum = 1;
      break;
    case 0x39D41DC6:
      buttonNum = 2;
  }
  
  if (buttonNum != -1)
  {
    if (!irButtonHeld)
    {
      irButtonDown(buttonNum);
    }
    else if ((buttonNum != lastIRButtonNumPressed) && irButtonHeld)
    {
      irButtonUp();
    }
    
    lastIRButtonNumPressed = buttonNum;
    lastIRButtonPressedTime = millis();
  }
}

void MonsoonRS400::irButtonDown(int buttonNum)
{
  irButtonHeld = true;
  irButtonBeingHeld = buttonNum;
  irButtonHeldStartTime = millis();
  
  buttonDown(buttonNum);
}

void MonsoonRS400::irButtonUp()
{
  int tempButtonNum = irButtonBeingHeld;
  irButtonHeld = false;
  irButtonBeingHeld = -1;
  irButtonHeldStartTime = millis();
  
  buttonUp(tempButtonNum);
}

bool MonsoonRS400::isIRButtonHeld(int holdTime)
{
  bool retval = false;
  if (irButtonHeld)
  {
    if (((millis() - irButtonHeldStartTime) > holdTime) && !irButtonWaitingForRelease)
    {
      irButtonWaitingForRelease = true;
      retval = true;
    }
    else if ((millis() - lastIRButtonPressedTime) > IRKEYUPTIME)
    {
      irButtonWaitingForRelease = false;
      retval = false;
      irButtonUp();
    }
  }
  
  return retval;
}
