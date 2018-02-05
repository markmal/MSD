/*
 * SCSIDevice.cpp
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */

#include <SCSIDevice.h>

#include <stdint.h>
#include "my_debug.h"
#include "SCSI.h"
#include "MSC.h"
#include <SD.h>
//#include <Sd2Card.h>

SCSIDevice::SCSIDevice() {
	// TODO Auto-generated constructor stub

	sdCard = new Sd2Card();
	sdCard->init();
	sdCardErrorCode = sdCard->errorCode();
	sdCardErrorData = sdCard->errorData();
	LUN=0;
	memset(blockData,0,512);
	memset(inquiryData.array, 0,sizeof(inquiryData));
	memset(capacity10.array , 0,sizeof(capacity10));
}

SCSIDevice::~SCSIDevice() {
	// TODO Auto-generated destructor stub
	delete sdCard;
}

int SCSIDevice::processInquiry(SCSI_CBD_INQUIRY  &cbd, uint8_t* &data, uint8_t& len) {
	memset(&inquiryData, 0, sizeof(inquiryData));
	inquiryData.inquiry.peripheral_device_type = SCSI_SBC2;
	inquiryData.inquiry.RMB = 1;
	inquiryData.inquiry.version = 0; // The device does not claim conformance to any standard.
	inquiryData.inquiry.additional_length = sizeof(inquiryData) - 4;
	uint8_t v[8] = {'M','M','7',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.t10_vendor_id, v, 8);
	uint8_t p[16] = {'S','V','M','L',' ','A','F','M','0',' ',' ',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.product_id, p, 16);
	uint8_t r[4] = {'0','0','0','1'};
	memcpy(inquiryData.inquiry.product_revision_level, r, 4);
	uint8_t s[8] = {'1','2','3','4','5','6','7','8'};
	memcpy(inquiryData.inquiry.drive_serial_number, s, 8);
	data = inquiryData.array;
	len = sizeof(inquiryData);
	return len;
}

int SCSIDevice::processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint8_t* &data, uint8_t& len) {
	//TODO Check if SD is ready
	if (!sdCard) return NO_MEDIA;
	//if (!(sdCard->type() waitNotBusy(SD_INIT_TIMEOUT))) return MEDIA_BUSY;
	data = NULL;
	len = 0;
	return MEDIA_READY;
}

int SCSIDevice::processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint8_t* &data, uint8_t& len) {
	uint32_t sz = sdCard->cardSize();
	capacity10.fields.lastLBA  = sz - 1;
	capacity10.fields.block_sz = 512; // 66912448 * 512 = 32GB
	debug += "  capacity\n";
	data = capacity10.array;
	len = sizeof(capacity10);
	return len;
}

int SCSIDevice::processRead10(SCSI_CBD_READ_10  &cbd, uint8_t* &data, uint8_t& len) {
	uint8_t r = sdCard->readBlock(cbd.LBA, blockData);
	if (r) {
		data = blockData;
		len = (uint8_t)512;
		return len;
	}
	else {
		data = NULL;
		len = 0;
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		return MEDIA_READ_ERROR;
	}
}

int SCSIDevice::processWrite10(SCSI_CBD_WRITE_10  &cbd, uint8_t* &data, uint8_t& len) {

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
		return MEDIA_READ_ERROR;
	}
	*/
	return 512;
}

int SCSIDevice::processRequest(SCSI_CBD &cbd, uint8_t* &data, uint8_t& len){
	debug += "SCSIDevice::processRequest\n";
	if (cbd.generic.opcode == SCSI_READ_10){
		debug += "  READ\n";
		int r = processRead10(cbd.read10, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_WRITE_10){
		debug += "  WRITE\n";
		int r = processWrite10(cbd.write10, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_INQUIRY){
		debug += "  INQUIRY\n";
		int r = processInquiry(cbd.inquiry, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_TEST_UNIT_READY){
		debug += "  TEST_UNIT_READY\n";
		int r = processTestUnitReady(cbd.unit_ready, data, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_CAPACITY_10){
		debug += "  READ_CAPACITY\n";
		int r = processReadCapacity10(cbd.read_capacity10, data, len);
		return r;
	};

	return SCSI_UNSUPPORTED_OPERATION;
}


