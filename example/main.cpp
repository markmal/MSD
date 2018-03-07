/*
 * main.cpp
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#include <Arduino.h>
#include "LcdConsole.h"
#include <MSCDeviceClass.h>
#include <USB/USBAPI.h>


void blink2(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}

//#define SD_DEBUG 1
//#define MAIN_DEBUG
#define LCD_DEBUG 1

#ifdef MAIN_DEBUG
  #ifdef CDC_ENABLED
	bool isDebug = false;
	Serial_ Serial = Serial_(USBDevice);
	#define dbgout(str) if(isDebug)Serial.print(str)
	#define dbgoutln(str) if(isDebug)Serial.println(str)
  #else
	// just do nothing
	#define dbgout(str)
	#define dbgoutln(str)
  #endif
#elif SD_DEBUG
	#include <SD.h>
	File debugFile;
	#define dbgout(str) debugFile.print(str);debugFile.flush()
	#define dbgoutln(str) debugFile.println(str);debugFile.flush()
#elif LCD_DEBUG
	#include "LcdConsole.h"
	#define dbgout(str) lcdConsole.prints(str)
	#define dbgoutln(str) lcdConsole.prints(str)
#else
	// just do nothing
	#define dbgout(str)
	#define dbgoutln(str)
#endif


//uint8_t AAA[4096];
MSCDeviceClass MSC;

void setup() {
	//blink2(1000);
#ifdef CDC_ENABLED
	#ifdef  MAIN_DEBUG
		Serial.begin(115200);
		while (!Serial.available() && millis()<4000);
		if (Serial.available()){
			int i = Serial.read();
			isDebug = true;
			dbg.println("DEBUG");
		}

	#endif
#endif

#ifdef SD_DEBUG
	SD.begin(4); // 4!
	delay(1000);
	debugFile=SD.open("DEBUG.TRC",FILE_WRITE);
#endif

#ifdef LCD_DEBUG
	lcdConsole.begin();
#endif
	dbgoutln("setup():"+debugGet()+":Endsetup");

	if (! MSC.begin()) dbgoutln(MSC.getError());

	//if (debugLength()>2) print("debug:"+debugGet());
	debugClear();
}


bool readUsb = true;

bool LedOn = false;
long int LedOnMillis = 0;
String ri;


void loop() {
  uint32_t i=0;

  if (readUsb)
  do {

	i = MSC.receiveRequest();
	if (debugLength()>2) dbgout("debug:"+debugGet());
	debugClear();

	if (!LedOn){
		digitalWrite(LED_BUILTIN, HIGH);
		LedOn = true;
		LedOnMillis = millis();
	}
	ri=MSC.getSCSIRequestInfo();
	//if (ri.length()>0) println( MSC.getSCSIRequestInfo() );

	//if (i<0) println(MSC.getError());


	delay(1);
  } while (i != 0);

  if (LedOn && ((millis() - LedOnMillis) >= 100 )){
		digitalWrite(LED_BUILTIN, LOW);
		LedOn = false;
  }

  #ifdef SD_DEBUG
	debugFile.close();
	debugFile=SD.open("DEBUG.TRC",FILE_WRITE);
   #endif

  delay(50);
}

