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
const char* debugGetC();

void debugClear();

void debugPrint(String s);

void debugPrintln(String s);

void debugPrintlnSI(String s, int i);

void debugPrintlnSC(String s, char* cc, int len);

void debugPrintlnSX(String s, uint8_t* cc, int len);

// fake dummy class
#define DUMMY_SD
#ifdef DUMMY_SD

#ifndef Sd2Card_h
uint8_t const DUMMY_SPI_FULL_SPEED = 0;
#endif

class Sd2CardDummy {
public:
	Sd2CardDummy(){};
	uint8_t init(uint8_t sckRateID, uint8_t chipSelectPin){return true;};
	uint8_t type(){return 0;};
	uint64_t cardSize(){return 8000000000;};
	//void readCID(cid_t &cid){};
	uint8_t errorCode(void) const {return 0;}
    uint8_t errorData(void) const {return 0;}
	uint8_t writeBlock(uint32_t LBA, uint8_t* data){return (uint8_t)1;};
	uint8_t readBlock(uint32_t LBA, uint8_t* data){return (uint8_t)1;};
};
#endif

typedef struct { uint16_t len; char str[1022]; } CharsPage;

#endif /* SRC_MY_DEBUG_H_ */
