/*
 * main.cpp
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#include <Arduino.h>
#include "MSC.h"

void blink2(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}

MSC_ MSC;

void setup() {
	debug += "START\n";
  Serial.begin(115200);
  delay(3000);
}

void loop() {
  uint32_t i=0;
  do {
	blink2(50);
    i = MSC.receiveBlock();
    if (i != 0) {
      Serial.println("Received: ");
    }

    if ( debug.length() == 0) Serial.println(".");
    else {
    	Serial.print(debug);
    	debug="";
    }
    delay(1000);
  } while (i != 0);
}

