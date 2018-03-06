/*
 * main.cpp
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#include <Arduino.h>
#include <MSCDeviceClass.h>
#include <USB/USBAPI.h>
#include "LcdConsole.h"


void blink2(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}

#define SD_DEBUG 1
//#define MAIN_DEBUG

#ifdef MAIN_DEBUG
  #ifdef CDC_ENABLED
	bool isDebug = false;
	Serial_ Serial = Serial_(USBDevice);
	#define print(str) if(isDebug)Serial.print(str)
	#define println(str) if(isDebug)Serial.println(str)
  #else
	// just do nothing
	#define print(str)
	#define println(str)
  #endif
#elif SD_DEBUG
	#include <SD.h>
	File debugFile;
	#define print(str) debugFile.print(str);debugFile.flush()
	#define println(str) debugFile.println(str);debugFile.flush()
#else
	// just do nothing
	#define print(str)
	#define println(str)
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
			println("DEBUG");
		}

	#endif
#endif

#ifdef SD_DEBUG
	SD.begin(4); // 4!
	delay(1000);
	debugFile=SD.open("DEBUG.TRC",FILE_WRITE);
#endif
	print("Debug:"+debugGet());

	if (! MSC.begin()) println(MSC.getError());

	debugPrintln("START");
	//lcdConsole.begin();
	println("START");

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

	if (!LedOn){
		digitalWrite(LED_BUILTIN, HIGH);
		LedOn = true;
		LedOnMillis = millis();
	}
	ri=MSC.getSCSIRequestInfo();
	if (ri.length()>0) println( MSC.getSCSIRequestInfo() );

	if (i<0) println(MSC.getError());

	if (debugLength()>2) print("debug:"+debugGet());
	void debugClear();

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

