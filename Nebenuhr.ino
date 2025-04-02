/*
 * Nebenuhr
 
used I2C-Addresses:
  - 0x20 LCD-Panel
  - 0x39 Tastenadapter
  - 0x3D FastClock-Interface
  - 0x70 FastClock-LED-Display
  
discrete In/Outs used for functionalities:
  -  0    (used  USB)
  -  1    (used  USB)
  -  2
  -  3 Out used   Segment A
  -  4 Out used   Segment B
  -  5 Out used   Segment C
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9 Out used   Segment D
  - 10 Out used   Segment E
  - 11 Out used   Segment F 
  - 12 Out used   Segment G
  - 13 Out used   Segment DP
  - 14 Out used   Common Anode Display 1 (MSD (most significant Digit)
  - 15 Out used   Common Anode Display 2
  - 16 Out used   Common Anode Display 3
  - 17 Out used   Common Anode Display 4 (LSD (last significant Digit)
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2019 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor
                  // note: activating SerialMonitor in Arduino-IDE
                  //       will create a reset in software on board!

//#define DEBUG_CV 1  // enables CV-Output to serial port during program start (saves 180Bytes of code :-)
//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define FAST_CLOCK_LOCAL 1  //use local ports 2 and 3 as slaveclockmodule as an alternative/additional for local I2C-Module
                              // not usable with this hardware, ports already used for LED

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
                                // instead from LocoNet-Port (which is inactive then)

#define LCD     // used in GlobalOutPrint.ino

#include "CV.h"

#define BA_MODUS_FC_SLAVE     (GetCV(ID_BA_MODUS) == 0)
#define BA_MODUS_DECODER      (GetCV(ID_BA_MODUS) == 1)
#define BA_MODUS_DIRECT       (GetCV(ID_BA_MODUS) == 2)

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)
#define ENABLE_LN_FC_MODUL    (BA_MODUS_FC_SLAVE || GetCV(ADD_FUNCTIONS_1) & 0x04)
#define ENABLE_LN_FC_INTERN   (GetCV(ADD_FUNCTIONS_1) & 0x08)
#define ENABLE_LN_FC_INVERT   (GetCV(ADD_FUNCTIONS_1) & 0x20)
#define ENABLE_LN_FC_SLAVE    (ENABLE_LN_FC_MODUL)
#define ENABLE_LN_FC_JMRI     (GetCV(ADD_FUNCTIONS_1) & 0x10)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID  13   // NMRA: DIY
#define DEVELOPER_ID  58      // NMRA: my ID, should be > 27 (1 = FREMO, see https://groups.io/g/LocoNet-Hackers/files/LocoNet%20Hackers%20DeveloperId%20List_v27.html)

//=== declaration of var's =======================================

#include <LocoNet.h>    // requested for notifyFastClock

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  ReadCVsFromEEPROM();
  
  CheckAndInitLCDPanel();

  InitLocoNet();
  
  if(ENABLE_LN_FC_SLAVE)
    InitFastClock();
	
  InitClockHandling();
}

void loop()
{
  // light the Heartbeat LED
  oHeartbeat.beat();
  // generate blinken
  Blinken();

  //=== do LCD handling ==============
  // can be connected every time
  // panel only necessary for setup CV's (or some status informations):
  HandleLCDPanel();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

	//=== do FastClock handling ===
	HandleFastClock();

  //=== do Clock handling ===
  HandleClockHandling();
	
#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

/*=== will be called from LocoNetFastClockClass
			if telegram is OPC_SL_RD_DATA [0xE7] or OPC_WR_SL_DATA [0xEF] and clk-state != IDLE ==================*/
void notifyFastClock( uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync )
{
#if defined DEBUG
  Serial.println();
  Serial.print(F("notifyFastClock "));
  Serial.print(Hour);
  Serial.print(":");
  decout(Serial, Minute, 2);
  Serial.print("[Rate=");
  Serial.print(Rate);
  Serial.print("][Sync=");
  Serial.print(Sync);
	Serial.println("]");
#endif    
  SetFastClock(Rate, Day, Hour, Minute, Sync);
}

void notifyFastClockFracMins(uint16_t FracMins)
{
  HandleFracMins(FracMins);
}
