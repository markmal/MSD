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
	dd.array[1] = ss.array[1];
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
uint16_t msb2lsb(uint16_t v){
	uint8_t src[2] = (uint8_t*)&v;
	uint8_t rsl[2];
	rsl[0] = src[1];
	rsl[1] = src[2];
	return rsl;
}

uint32_t msb2lsb(uint32_t v){
	uint8_t src[4] = (uint8_t*)&v;
	uint8_t rsl[4];
	rsl[0] = src[3];
	rsl[1] = src[2];
	rsl[2] = src[1];
	rsl[3] = src[0];
	return rsl;
}

uint64_t msb2lsb(uint64_t v){
	uint8_t src[8] = (uint8_t*)&v;
	uint8_t rsl[8];
	rsl[0] = src[7];
	rsl[1] = src[6];
	rsl[2] = src[5];
	rsl[3] = src[4];
	rsl[4] = src[3];
	rsl[5] = src[2];
	rsl[6] = src[1];
	rsl[7] = src[0];
	return rsl;
}
*/



