/*
 * main.cpp
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#include <Arduino.h>
#include <MSCDeviceClass.h>
#include <USB/USBAPI.h>
#include "my_debug.h"

void blink2(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}

MSCDeviceClass MSC;

uint8_t AAA[4096];

void setup() {
	debug += "START\n";
  /*
  Serial.begin(115200);
  delay(3000);

  for(int i=0; i< 4096; i++) {
		AAA[i] = (i%16)+0x41;
  }
  Serial.write(AAA,4096);
  Serial.flush();
  Serial.println();
  Serial.println("DONE");
  */
}

void loop() {
  uint32_t i=0;
  do {
	blink2(50);
    i = MSC.receiveRequest();
    if (i != 0) {
      //Serial.println("Received: ");
    }

	/*if ( usbdebug.length() == 0) Serial.println(".");
	    else {
	    	//Serial.print(usbdebug);
	    	usbdebug="";
	    	//Serial.flush();
	    }*/
   	usbdebug="";

	/*if ( debug.length() == 0) Serial.println(".");
    else {
    	Serial.print(debug);
    	debug="";
    	Serial.flush();
    }*/
	debug="";
    delay(10);
  } while (i != 0);
  delay(1000);
}

