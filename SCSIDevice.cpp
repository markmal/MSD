/*
 * SCSIDevice.cpp
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */


#include <stdint.h>
#include "my_debug.h"
#include "SCSI.h"
#include "MSC.h"
//#include <SD.h>
//#include <utility/Sd2Card.h>

#include "SCSIDevice.h"

SCSIDevice::SCSIDevice():
	lastLBA(0), blockSize(SD_BLOCK_SIZE), sdCardErrorCode(0), sdCardErrorData(0){
/*	sdCard = new Sd2Card();
	sdCard->init();
	sdCardErrorCode = sdCard->errorCode();
	sdCardErrorData = sdCard->errorData();
	*/
	memset(blockData,0,512);
	memset(inquiryData.array, 0,sizeof(inquiryData));
	memset(capacity10.array , 0,sizeof(capacity10));
}

SCSIDevice::~SCSIDevice() {
	// TODO Auto-generated destructor stub
	//delete sdCard;
}

int SCSIDevice::processInquiry(SCSI_CBD_INQUIRY  &cbd, uint8_t* &data, uint16_t& len) {
	memset(&inquiryData, 0, sizeof(inquiryData));
	inquiryData.inquiry.peripheral_device_type = SCSI_SBC2;
	inquiryData.inquiry.RMB = 1;
	inquiryData.inquiry.version = SCSI_SPC2; // SPC-2
	inquiryData.inquiry.additional_length = 32;
	uint8_t v[8] = {'M','M','7',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.t10_vendor_id, v, 8);
	uint8_t p[16] = {'S','V','M','L',' ','A','F','M','0',' ',' ',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.product_id, p, 16);
	uint8_t r[4] = {'0','0','0','1'};
	memcpy(inquiryData.inquiry.product_revision_level, r, 4);
	uint8_t s[8] = {'1','2','3','4','5','6','7','8'};
	memcpy(inquiryData.inquiry.drive_serial_number, s, 8);
	data = inquiryData.array;
	if (len>sizeof(inquiryData))
		len = sizeof(inquiryData);
	return len;
}

int SCSIDevice::processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint8_t* &data, uint16_t& len) {
	//TODO Check if SD is ready
	///if (!sdCard) return NO_MEDIA;
	//if (!(sdCard->type() waitNotBusy(SD_INIT_TIMEOUT))) return MEDIA_BUSY;
	data = NULL;
	len = 0;
	return MEDIA_READY;
}

int SCSIDevice::processModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint8_t* &data, uint16_t& len) {
	if (cbd.PC == 0){ // current values
		if (cbd.page_code == 0x3F)
			if (cbd.subpage_code == 0x00 ) {
				modesenseData6.fields.mode_data_length = 3;
				modesenseData6.fields.medium_type = 0x00;
				modesenseData6.fields.dev_specific_param  = 0x00;
				modesenseData6.fields.block_descr_length = 0;
			}
	}
	data = modesenseData6.array;
	len = 4;
	return 4;
}

int SCSIDevice::processMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint8_t* &data, uint16_t& len){
	// if (sdCard in ) return 0
	// else return ERROR_NO_MEDIA
	//data =
	//len = ;
	return 0;
}

int SCSIDevice::processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint8_t* &data, uint16_t& len) {
	///uint32_t sz = sdCard->cardSize();
	///capacity10.fields.lastLBA  = sz - 1;
	lastLBA = 0xFFFFF;
	SCSI_CAPACITY_DATA_10 cd;
	cd.fields.lastLBA = lastLBA;
	cd.fields.block_sz = blockSize;

	msb2lsb(cd.fields.lastLBA, capacity10.fields.lastLBA);
	msb2lsb(cd.fields.block_sz, capacity10.fields.block_sz);

	debugPrintlnSX("  capacity:", capacity10.array, len);
	data = capacity10.array;
	len = sizeof(capacity10);
	return len;
}

uint16_t toUint16(uint8_t a[2]){
	return a[0]<<8 | a[1];
}
uint32_t toUint32(uint8_t a[4]){
	return a[0]<<24 | a[1]<<16 | a[2]<<8 | a[3];
}

int SCSIDevice::processRead10(SCSI_CBD_READ_10 &cbd, uint8_t* &data, uint16_t& len) {
	uint32_t LBA = toUint32(cbd.LBA_a);
	uint16_t txlen = toUint16(cbd.length_a);

	//uint8_t r = 1; //sdCard->readBlock(cbd.LBA, blockData);

	//msb2lsb(cbd.LBA, LBA);
	//msb2lsb(cbd.length, txlen);
	debug+=" read LBA:"+String(LBA)+" txlen:"+String(txlen)+"\n";

	//if (LBA > lastLBA) error();
	//if (cbd.length == 0) len=0;
	for(int i=0; i< len; i++) {
		blockData[i] = (i%16)+0x41;
	}
	data = blockData;
	len = txlen * blockSize;
	return len;

	/*if (r) {
		//data = blockData;
		//len = txlen;
		return len;
	}
	else {
		data = NULL;
		len = 0;
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		return ERROR_MEDIA_READ;
	}
	*/
}

int SCSIDevice::processWrite10(SCSI_CBD_WRITE_10  &cbd, uint8_t* &data, uint16_t& len) {

	/*
	uint8_t r = sdCard->writeBlock(cbd.LBA, blockData);
	if (r) {
		data = blockData;
		len = 512;
		return len;
	}
	else {
		data = NULL;
		len = 0;
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		return ERROR_MEDIA_READ;
	}
	*/
	return 512;
}


int SCSIDevice::processRequest(SCSI_CBD &cbd, uint8_t* &data, uint16_t& len){
	debug += "SCSIDevice::processRequest\n";
	if (cbd.generic.opcode == SCSI_READ_10){
		debug += "  READ10\n";
		debugPrintlnSX("  cbd:",cbd.array, 10);
		int r = processRead10(cbd.read10, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_WRITE_10){
		debug += "  WRITE10\n";
		int r = processWrite10(cbd.write10, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_TEST_UNIT_READY){
		debug += "  TEST_UNIT_READY\n";
		int r = processTestUnitReady(cbd.unit_ready, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_INQUIRY){
		debug += "  INQUIRY\n";
		int r = processInquiry(cbd.inquiry, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_CAPACITY_10){
		debug += "  READ_CAPACITY_10\n";
		int r = processReadCapacity10(cbd.read_capacity10, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_MODE_SENSE_6){
		debug += "  MODE_SENSE_6\n";
		int r = processModeSense6(cbd.mode_sense6, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL){
		debug += "  SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL\n";
		int r = processMediumRemoval(cbd.medium_removal, data, len);
		return r;
	};


	return SCSI_UNSUPPORTED_OPERATION;
}


