/*
 * my_debug.h
 *
 *  Created on: Jan 30, 2018
 *      Author: mark
 */

#ifndef SRC_MY_DEBUG_H_
#define SRC_MY_DEBUG_H_

#include <stdint.h>
#include <Arduino.h>

//extern String debug;

int debugLength();

String debugGet();

void debugClear();

void debugPrint(String s);

void debugPrintln(String s);

void debugPrintlnSI(String s, int i);

void debugPrintlnSC(String s, char* cc, int len);

void debugPrintlnSX(String s, uint8_t* cc, int len);

#endif /* SRC_MY_DEBUG_H_ */
