/*
 * my_debug.cpp
 *
 *  Created on: Jan 30, 2018
 *      Author: mark
 */

#include <Arduino.h>
#include "my_debug.h"

String debug = "DEBUG\n";

void debugPrintlnSI(String s, int i){
	debug += s + String(i) + "\n";
}

void debugPrintlnSC(String s, char* cc, int len){
	debug += s;
	for (int i=0; i<len; i++) debug += cc[i];
	debug += "\n";
}

String a2x(uint8_t* cc, int len){
	String r="";
	for (int i=0; i<len; i++) {
		if (i % 8 == 0) r += " ";
		if (i % 16 == 0) r += "\n";
		r += ((cc[i]<16)?"0":"") + String(cc[i],16)+" ";
	}
	return r;
}

void debugPrintlnSX(String s, uint8_t* cc, int len){
	debug += s + String(len);
	for (int i=0; i<len; i++) {
		if (i % 8 == 0) debug += " ";
		if (i % 16 == 0) debug += "\n";
		debug += ((cc[i]<16)?"0":"") + String(cc[i],16)+" ";
	}
	debug += "\n";
}
