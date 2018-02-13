/*
 * SCSI.cpp
 *
 *  Created on: Jan 30, 2018
 *      Author: mark
 */

#include <stdint.h>
//#include "my_debug.h"
//#include "SCSI.h"
//#include "MSC.h"

union LSB16{
	uint16_t uint16;
	uint8_t array[2];
};

void msb2lsb(uint16_t& s, uint16_t& d){
	LSB16& ss = (LSB16&)s;
	LSB16& dd = (LSB16&)d;
	dd.array[1] = ss.array[0];
	dd.array[0] = ss.array[1];
}

void msb2lsb(uint32_t& s, uint32_t& d){
	LSB16& ss = (LSB16&)s;
	LSB16& dd = (LSB16&)d;
	dd.array[0] = ss.array[3];
	dd.array[1] = ss.array[2];
	dd.array[2] = ss.array[1];
	dd.array[3] = ss.array[0];
}

void msb2lsb(uint64_t& s, uint64_t& d){
	LSB16& ss = (LSB16&)s;
	LSB16& dd = (LSB16&)d;
	dd.array[0] = ss.array[7];
	dd.array[1] = ss.array[6];
	dd.array[2] = ss.array[5];
	dd.array[3] = ss.array[4];
	dd.array[4] = ss.array[3];
	dd.array[5] = ss.array[2];
	dd.array[6] = ss.array[1];
	dd.array[7] = ss.array[0];
}

/*
 * returns uint16_t from MSB-LSB array
 */
uint16_t toUint16(uint8_t a[2]){
	return a[0]<<8 | a[1];
}

/*
 * returns uint32_t from MSB-LSB array
 */
uint32_t toUint32(uint8_t a[4]){
	return a[0]<<24 | a[1]<<16 | a[2]<<8 | a[3];
}


