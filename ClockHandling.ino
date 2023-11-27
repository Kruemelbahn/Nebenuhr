//=== ClockHandling for Nebenuhr ===
#include <Wire.h>
#include "BounceSimplepcf.h"

#include "4x7Segment.h"

//=== declaration of var's =======================================
SevenSegment sevenSeg = SevenSegment();

boolean bHour = false;
boolean bMinute = false;
uint8_t ui8_Hour = 0;
uint8_t ui8_Minute = 0;

unsigned long ul_StartTime = 0;

#define PCF8574A_ADDR_KEY (0x39)
#define DEBOUNCE_TIME 5

BounceSimplePcf debouncer_Hour; 
BounceSimplePcf debouncer_Min; 
BounceSimplePcf debouncer_Inc10; 
BounceSimplePcf debouncer_Inc1; 

BounceSimplePcf debouncer_Decoder; 
BounceSimplePcf debouncer_Direct; 

//=== Functions =======================================
boolean getStateDecoder() { return debouncer_Decoder.read(); }
boolean getStateDirect()  { return debouncer_Direct.read(); }

void CheckAndInitKeypad()
{
  Wire.begin();

  //---keypad--------------
  Wire.beginTransmission(PCF8574A_ADDR_KEY);
  boolean b_keypadDetected(Wire.endTransmission() == 0);

  if(b_keypadDetected)
  {
#if defined DEBUG
    Serial.print(F("Keypad found, Adr:"));
    Serial.println(PCF8574A_ADDR_KEY);
#endif

    debouncer_Hour.attach (PCF8574A_ADDR_KEY << 1, 7, DEBOUNCE_TIME); 
    debouncer_Min.attach  (PCF8574A_ADDR_KEY << 1, 6, DEBOUNCE_TIME); 
    debouncer_Inc10.attach(PCF8574A_ADDR_KEY << 1, 5, DEBOUNCE_TIME); 
    debouncer_Inc1.attach (PCF8574A_ADDR_KEY << 1, 4, DEBOUNCE_TIME); 

    debouncer_Decoder.attach(PCF8574A_ADDR_KEY << 1, 1, DEBOUNCE_TIME); 
    debouncer_Direct.attach (PCF8574A_ADDR_KEY << 1, 0, DEBOUNCE_TIME); 

  } // if(b_keypadDetected)
  bHour = bMinute = false;
	ul_StartTime = 0;
}

void updateDebounce()
{
  // Update the Bounce instances :
  debouncer_Hour.update();
  debouncer_Min.update();
  debouncer_Inc10.update();
  debouncer_Inc1.update();

  debouncer_Decoder.update();
  debouncer_Direct.update();
}

void InitClockHandling()
{
  CheckAndInitKeypad();
  sevenSeg.init(4, true, true); // 4 digits - leading zero - rightbound
}

void showTime()
{
  if ((ui8_Hour > 23 || ui8_Minute > 59))
      sevenSeg.showErr();
  sevenSeg.printChar(3, (ui8_Minute % 10) + '0');
  sevenSeg.printChar(2, (ui8_Minute / 10) + '0');
  sevenSeg.printChar(1, (ui8_Hour % 10) + '0');
  sevenSeg.printChar(0, ui8_Hour > 9 ? (ui8_Hour / 10) + '0' : ' ');
  sevenSeg.setDP(3, isFastClockRunning() && Blinken2Hz());
}

void HandleClockHandling()
{
  boolean bIsFastClock(GetFastClock(&ui8_Hour, &ui8_Minute));
  if(BA_MODUS_FC_SLAVE)
  {
    if(!bIsFastClock)
      sevenSeg.showDash();
    else
      showTime();
  }
  else
  {
    { // handle buttons:
      updateDebounce();

      if(debouncer_Hour.rose())
      {
        bHour = true;
        bMinute = false;
      }
      if(debouncer_Hour.fell())
        bHour = bMinute = false;
  
      if(debouncer_Min.rose())
      {
        bHour = false;
        bMinute = true;
      }
      if(debouncer_Min.fell())
        bHour = bMinute = false;

      // reset of time requested?
			if (debouncer_Inc1.fell())
				ul_StartTime = 0;
			if ((ul_StartTime > 0) && ((millis() - ul_StartTime) > 2000))
			{
				ul_StartTime = 0;
				if (bHour)
					ui8_Hour = 0;
				if (bMinute)
					ui8_Minute = 0;
			}

      // inc hour up to 23
			if(bHour)
      {
				if(debouncer_Inc10.rose())
          ui8_Hour += 10;
        if(debouncer_Inc1.rose())
        {
					ul_StartTime = millis();
          ui8_Hour += 1;
        }
        if(ui8_Hour > 23)
          ui8_Hour = 0;
      }
      // inc minute up to 59
      if(bMinute)
      {
        if(debouncer_Inc10.rose())
          ui8_Minute += 10;
        if(debouncer_Inc1.rose())
				{
					ul_StartTime = millis();
					ui8_Minute += 1;
				}
        if(ui8_Minute > 59)
          ui8_Minute = 0;
      }
    } // handle buttons

    { // handle external pulse:
      if(BA_MODUS_DECODER)
      {
        if(debouncer_Decoder.rose())
          ui8_Minute += 1;
        if(debouncer_Decoder.fell())
          ui8_Minute += 1;
      }

      if(BA_MODUS_DIRECT)
      {
        if(debouncer_Direct.rose())
          ui8_Minute += 1;
        if(debouncer_Direct.fell())
          ui8_Minute += 1;
      }

      if(ui8_Minute > 59)
      {
        ui8_Minute = 0;
        ui8_Hour += 1;
        if(ui8_Hour > 23)
          ui8_Hour = 0;
      }
    } // handle external pulse

    uint8_t ui8_HourActual, ui8_MinuteActual;
    GetFastClock(&ui8_HourActual, &ui8_MinuteActual);
    if((ui8_HourActual != ui8_Hour) || (ui8_MinuteActual != ui8_Minute))
      SetFastClock(1, 1, ui8_Hour, ui8_Minute, 1);
      
    showTime();
  }
}
