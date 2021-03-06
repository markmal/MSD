/*
 * SCSIDeviceClass.cpp
 *
 *  Created on: Feb 1, 2018
 *      Author: mark malakanov
 */
/*
  Copyright (c) 2018 Mark Malakanov.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "MSCDeviceClass.h"
#include <stdint.h>
#include <USB/USBAPI.h>
#include "SCSIDeviceClass.h"
#include "my_debug.h"
#include "SCSI.h"
#include "LcdConsole.h"


#ifdef SCSI_DEVICE_CLASS_DEBUG
	#define print(str) Serial.print(str)
	#define println(str) Serial.println(str)
#else
	#define print(str)
	#define println(str)
#endif


#ifdef SCSI_DEVICE_CLASS_LCD_DEBUG
	#define print(str) lcdConsole.print(str)
	#define println(str) lcdConsole.println(str)
#else
	#define print(str)
	#define println(str)
#endif


SCSIDeviceClass::SCSIDeviceClass():
	lastLBA(0), LBA(0), blockSize(SD_BLOCK_SIZE),
	senseKey(0), sdCardErrorCode(0), sdCardErrorData(0),
	scsiStatus(GOOD)
{
	//sdCard = NULL;

	transferLength = 0;
	txLBA=0;
	txLBAcnt=0;
	txLen=0;
	LBAcnt=0;

	isWriteProtected = false;

	additionalSenseCodeQualifier=0;
	memset(senseInformation,0,4);
	additionalSenseCode=0;
	additionalSenseCodeQualifier=0;
	incorrectLengthIndicator=false;
	isSdCardReady=false;

	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	maxTransferLength = MAX_TRANSFER_LENGTH;
	maxTransferLBAcount = maxTransferLength / blockSize;
	transferData = (uint8_t*)malloc(MAX_TRANSFER_LENGTH);
	requestInfo = "";
	MSCResultCase = 0;

	memset(transferData, 0, MAX_TRANSFER_LENGTH);

#ifdef DUMMY_SD
	sdCard = new Sd2CardDummy();
#else
	sdCard = new Sd2Card();
#endif
}

SCSIDeviceClass::~SCSIDeviceClass() {
	// TODO Auto-generated destructor stub
	delete sdCard;
	free(transferData);
}

uint32_t SCSIDeviceClass::getMaxTransferLength(){
	return maxTransferLength;
}

int SCSIDeviceClass::handleStandardInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	//lcdConsole.println("Inquiry:"+ String(len));
	requestInfo+=" processInquiry len:"+String(len);
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	SCSI_STANDARD_INQUIRY_DATA inquiryData;
	memset(&inquiryData, 0, sizeof(inquiryData));
	inquiryData.peripheral_device_type = SCSI_SBC2;
	inquiryData.RMB = 1;
	inquiryData.version = SCSI_SPC2; // SPC-2
	inquiryData.additional_length = 32;
	uint8_t t10_vendor_id[8] = {'M','M','7',' ',' ',' ',' ',' '};
	memcpy(inquiryData.t10_vendor_id, t10_vendor_id, 8);
	uint8_t product_id[16] = {'S','V','M','L',' ','A','F','M','0',' ',' ',' ',' ',' ',' ',' '};
	memcpy(inquiryData.product_id, product_id, 16);
	uint8_t product_revision_level[4] = {'0','0','0','1'};
	memcpy(inquiryData.product_revision_level, product_revision_level, 4);
	uint8_t drive_serial_number[8] = {'1','2','3','4','5','6','7','8'};
	memcpy(inquiryData.drive_serial_number, drive_serial_number, 8);
	memcpy(transferData, &inquiryData, 36);
	transferLength = 36;
	return 36;
}

int SCSIDeviceClass::handleInquirySupportedVPDPages(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	SCSI_INQUIRY_SUPPORTED_VPD_PAGES_DATA supportedVPDPages;
	supportedVPDPages.peripheral_qualifier = 0;
	supportedVPDPages.peripheral_device_type = SCSI_SBC2;
	supportedVPDPages.page_code = 0x00;
	supportedVPDPages.reserv1 = 0x00;
	uint8_t supported_page_list[3] = {0x00, 0x80, 0x83};
	memcpy(supportedVPDPages.supported_page_list, supported_page_list, 3);
	size_t pl = sizeof(SCSI_INQUIRY_SUPPORTED_VPD_PAGES_DATA);
	supportedVPDPages.page_length = pl-3;
	memcpy(transferData, &supportedVPDPages, pl);
	transferLength = pl;
	return pl;
}

int SCSIDeviceClass::handleDeviceIdentification(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	SCSI_INQUIRY_DEVICE_IDENTIFICATION_DATA deviceIds;
	deviceIds.peripheral_qualifier = 0; // LUN
	deviceIds.peripheral_device_type = SCSI_SBC2;
	deviceIds.page_code = SCSI_INQUIRY_DEVICE_IDENTIFICATION_PAGE;
	deviceIds.reserv1 = 0x00;

	IDENTIFICATION_DESCRIPTOR_DATA ID1;
	ID1.protocol_identifier = 0x00; // like Kingston has
	ID1.association = 0;
	ID1.PIV = 1; //Protocol Identifier Valid
	ID1.identifier_length = 8;
	uint8_t identifier[8] = {'1','2','3','4','5','6','7','8'};
	memcpy(ID1.identifier, identifier, 8);

	deviceIds.identification_descriptor_list[0] = ID1;

	size_t pl = sizeof(deviceIds);
	deviceIds.page_length = pl - 3; //  N - 3

	memcpy(transferData, &deviceIds, pl);
	transferLength = pl;
	return pl;
}


int SCSIDeviceClass::handleInquiryUnitSerialNumberPage(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	SCSI_INQUIRY_UNIT_SERIAL_NUMBER_PAGE_DATA unitSerialNumberPage;
	unitSerialNumberPage.peripheral_qualifier = 0;
	unitSerialNumberPage.peripheral_device_type = SCSI_SBC2;
	unitSerialNumberPage.page_code = 0x80;
	unitSerialNumberPage.reserv1 = 0x00;
	unitSerialNumberPage.page_length = 16;
	uint8_t product_serial_number[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	memcpy(unitSerialNumberPage.product_serial_number, product_serial_number, 16);
	memcpy(transferData, &unitSerialNumberPage, 20);
	transferLength = 20;
	return 20;
}

int SCSIDeviceClass::handleInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	if(cbd.EVPD == 0) return handleStandardInquiry(cbd, len);
	if(cbd.EVPD == 1) {
		if (cbd.pgcode == SCSI_INQUIRY_SUPPORTED_VPD_PAGES_PAGES)
			return handleInquirySupportedVPDPages(cbd, len);
		if (cbd.pgcode == SCSI_INQUIRY_DEVICE_IDENTIFICATION_PAGE)
			return handleDeviceIdentification(cbd, len);
		if (cbd.pgcode == SCSI_INQUIRY_UNIT_SERIAL_NUMBER_PAGE)
			return handleInquiryUnitSerialNumberPage(cbd, len);
	}
	return FAILURE;
}



uint8_t  SCSIDeviceClass::SDCardType() {return sdCard->type();}
uint64_t SCSIDeviceClass::SDCardSize() {return sdCard->cardSize();}
String   SCSIDeviceClass::SDCardProductName() {
#ifndef DUMMY_SD
	cid_t cid;
	sdCard->readCID(&cid);
	//lcdConsole.println("Manuf Id:"+String(cid.mid));
	return String(cid.pnm);
#else
	return "DUMMY SD";
#endif
}
uint8_t  SCSIDeviceClass::SDCardErrorCode(){return sdCardErrorCode;}
uint8_t  SCSIDeviceClass::SDCardErrorData(){return sdCardErrorData;}
String  SCSIDeviceClass::getSDCardError(){
	String  s="no error";
#ifndef DUMMY_SD
	switch (sdCardErrorCode){
	case SD_CARD_ERROR_CMD0: s="timeout error for command CMD0"; break;
	case SD_CARD_ERROR_CMD8: s="CMD8 was not accepted - not a valid SD card"; break;
	case SD_CARD_ERROR_CMD17: s="card returned an error response for CMD17 (read block)"; break;
	case SD_CARD_ERROR_CMD24: s="card returned an error response for CMD24 (write block)"; break;
	case SD_CARD_ERROR_CMD25: s="WRITE_MULTIPLE_BLOCKS command failed"; break;
	case SD_CARD_ERROR_CMD58: s="card returned an error response for CMD58 (read OCR)"; break;
	case SD_CARD_ERROR_ACMD23: s="SET_WR_BLK_ERASE_COUNT failed"; break;
	case SD_CARD_ERROR_ACMD41: s="card's ACMD41 initialization process timeout"; break;
	case SD_CARD_ERROR_BAD_CSD: s="card returned a bad CSR version field"; break;
	case SD_CARD_ERROR_ERASE: s="erase block group command failed"; break;
	case SD_CARD_ERROR_ERASE_SINGLE_BLOCK: s="card not capable of single block erase"; break;
	case SD_CARD_ERROR_ERASE_TIMEOUT: s="Erase sequence timed out"; break;
	case SD_CARD_ERROR_READ: s="card returned an error token instead of read data"; break;
	case SD_CARD_ERROR_READ_REG: s="read CID or CSD failed"; break;
	case SD_CARD_ERROR_READ_TIMEOUT: s="timeout while waiting for start of read data"; break;
	case SD_CARD_ERROR_STOP_TRAN: s="card did not accept STOP_TRAN_TOKEN"; break;
	case SD_CARD_ERROR_WRITE: s="card returned an error token as a response to a write operation"; break;
	case SD_CARD_ERROR_WRITE_BLOCK_ZERO: s="attempt to write protected block zero"; break;
	case SD_CARD_ERROR_WRITE_MULTIPLE: s="card did not go ready for a multiple block write"; break;
	case SD_CARD_ERROR_WRITE_PROGRAMMING: s="card returned an error to a CMD13 status check after a write"; break;
	case SD_CARD_ERROR_WRITE_TIMEOUT: s="timeout occurred during write programming"; break;
	case SD_CARD_ERROR_SCK_RATE: s="incorrect rate selected"; break;
	}
#endif
	if (sdCardErrorCode) s += "("+String(sdCardErrorData,16)+"h)";
	return s;
}

String  SCSIDeviceClass::getSCSIError(){
	return "senseKey:"+String(senseKey)
			+" additionalSenseCode:0x"+String(senseKey,16)
			+" additionalSenseCodeQualifier:0x"+String(additionalSenseCodeQualifier,16)
			+" senseInformation:0x"+String(senseInformation[0],16)+String(senseInformation[1],16);
}

int SCSIDeviceClass::begin(){
	return initSD();
}

#ifndef Sd2Card_h
#define SPI_FULL_SPEED 0
#endif

int SCSIDeviceClass::initSD(){
	if (!sdCard->init(SPI_FULL_SPEED, chipSelect)) {
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		//lcdConsole.println("SD card initialization failed.");
		//lcdConsole.println("sdCardErrorCode:"+String(sdCardErrorCode));
		//lcdConsole.println("sdCardErrorData:"+String(sdCardErrorData));
		isSdCardReady=false;
		return FAILURE;
	}
	isSdCardReady=true;
	return GOOD;
}

/*String getSCSIError(){
	return "TODO"; //TODO
}*/

int SCSIDeviceClass::handleTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint32_t len) {
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	// Check if SD is ready

	if (!sdCard) {
		scsiStatus = CHECK_CONDITION;
		senseKey = HARDWARE_ERROR;
		requestInfo+=" !sdCard";
		return FAILURE;
	}

	if (sdCard->errorCode()) {
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		scsiStatus = CHECK_CONDITION;
		senseKey = UNIT_ATTENTION;
		additionalSenseCode = NOT_READY_TO_READY_CHANGE_MEDIUM_MAY_HAVE_CHANGED;
		additionalSenseCodeQualifier = NO_ASCQ;
		senseInformation[0] = sdCardErrorCode;
		senseInformation[1] = sdCardErrorData;
		requestInfo+=" ERROR";
		return FAILURE;
	}
	requestInfo+=" GOOD";
	transferLength = 0;
	return GOOD;
}

int SCSIDeviceClass::handleRequestSense(SCSI_CBD_REQUEST_SENSE  &cbd, uint32_t len) {
	//lcdConsole.println("ModeSense:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	SCSI_CBD_REQUEST_SENSE_DATA requestSenseData;
	size_t sz = sizeof(requestSenseData);
	memset(&requestSenseData,0,sz);
	if (cbd.desc == 0){ // return fixed sense data
		requestSenseData.valid = 1;
		requestSenseData.response_code = CURRENT_ERRORS;
		requestSenseData.ILI = incorrectLengthIndicator;
		requestSenseData.sense_key = senseKey;
		requestSenseData.additional_sense_len = 10; // ?
		memcpy(requestSenseData.information, senseInformation, 4);
		requestSenseData.additional_sense_code = additionalSenseCode;
		requestSenseData.additional_sense_code_qualifier = additionalSenseCodeQualifier;
	}
	else {
		requestSenseData.sense_key = NO_SENSE;
	}

	//int alloc_len = cbd.allocation_length;
	memcpy(transferData, &requestSenseData, sz);
	transferLength = sz;
	return sz;
}

int SCSIDeviceClass::handleModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint32_t len) {
	//lcdConsole.println("ModeSense:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	requestInfo+=" cbd.PC:x"+String(cbd.PC,16);
	requestInfo+=" cbd.page_code:x"+String(cbd.page_code,16);
	requestInfo+=" cbd.subpage_code:x"+String(cbd.subpage_code,16);
	SCSI_CBD_MODE_SENSE_DATA_6 modeSenseData6;
	int sz = sizeof(SCSI_CBD_MODE_SENSE_DATA_6);
	memset(&modeSenseData6, 0, sz);
	if (cbd.PC == 0){ // current values
		if (cbd.page_code == 0x3F) {
			if (cbd.subpage_code == 0x00 ) {
				modeSenseData6.mode_data_length = 3;
				modeSenseData6.medium_type = 0x00;
				modeSenseData6.dev_specific_param  = 0x00; // current
				if (isWriteProtected)
					modeSenseData6.dev_specific_param  = 0x80; // saved
				modeSenseData6.block_descr_length = 0;
			}
		}
	}
	transferLength = sz;
	memcpy(transferData, &modeSenseData6, sz);
	requestInfo+=" OK";
	return sz;
}

int SCSIDeviceClass::handleMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint32_t len){
	//lcdConsole.println("MediumRemoval:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	// if (sdCard in ) return 0
	// else return ERROR_NO_MEDIA
	transferLength = 0;
	return 0;
}

int SCSIDeviceClass::handleReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint32_t len) {
	//lcdConsole.println("ReadCapacity10:"+ String(len));
	///uint32_t sz = sdCard->cardSize();
	///capacity10.fields.lastLBA  = sz - 1;
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;

	lastLBA = sdCard->cardSize()-1;
	requestInfo+=" lastLBA:"+String(lastLBA);
	if (lastLBA < 0) return FAILURE;
	blockSize = 512;
	//if (sdCard->type() == 1) blockSize = 512;

	SCSI_CAPACITY_DATA_10 cd;
	msb2lsb(lastLBA, cd.lastLBA);
	msb2lsb(blockSize, cd.block_sz);
	memcpy(transferData, &cd, 8);
	transferLength = 8;
	return 8;
}

// This is first 512 block from a real SD card. Needed for FDisk and FAT. Remove after real SD card implementation
uint8_t BLOCK0[512] = {
		  0xfa, 0x33, 0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x8b, 0xf4, 0x50, 0x07,
		  0x50, 0x1f, 0xfb, 0xfc, 0xbf, 0x00, 0x06, 0xb9, 0x00, 0x01, 0xf2, 0xa5,
		  0xea, 0x1d, 0x06, 0x00, 0x00, 0xbe, 0xbe, 0x07, 0xb3, 0x04, 0x80, 0x3c,
		  0x80, 0x74, 0x0e, 0x80, 0x3c, 0x00, 0x75, 0x1c, 0x83, 0xc6, 0x10, 0xfe,
		  0xcb, 0x75, 0xef, 0xcd, 0x18, 0x8b, 0x14, 0x8b, 0x4c, 0x02, 0x8b, 0xee,
		  0x83, 0xc6, 0x10, 0xfe, 0xcb, 0x74, 0x1a, 0x80, 0x3c, 0x00, 0x74, 0xf4,
		  0xbe, 0xf8, 0x06, 0xac, 0x3c, 0x00, 0x74, 0x0b, 0x56, 0xbb, 0x07, 0x00,
		  0xb4, 0x0e, 0xcd, 0x10, 0x5e, 0xeb, 0xf0, 0xeb, 0xfe, 0xbf, 0x05, 0x00,
		  0x60, 0x6a, 0x00, 0x6a, 0x00, 0xff, 0x76, 0x0a, 0xff, 0x76, 0x08, 0x6a,
		  0x00, 0x68, 0x00, 0x7c, 0x6a, 0x01, 0x6a, 0x10, 0xb4, 0x42, 0xb2, 0x80,
		  0x8b, 0xf4, 0xcd, 0x13, 0x61, 0x61, 0x73, 0x0c, 0x33, 0xc0, 0xcd, 0x13,
		  0x4f, 0x75, 0xd9, 0xbe, 0xf8, 0x06, 0xeb, 0xbf, 0xbe, 0xf8, 0x06, 0xbf,
		  0xfe, 0x7d, 0x81, 0x3d, 0x55, 0xaa, 0x75, 0xb3, 0xbf, 0x52, 0x7c, 0x81,
		  0x3d, 0x46, 0x41, 0x75, 0x07, 0x47, 0x47, 0x80, 0x3d, 0x54, 0x74, 0x11,
		  0xbf, 0x03, 0x7c, 0x81, 0x3d, 0x4e, 0x54, 0x75, 0x40, 0x47, 0x47, 0x81,
		  0x3d, 0x46, 0x53, 0x75, 0x38, 0x60, 0xb4, 0x08, 0xb2, 0x80, 0xcd, 0x13,
		  0xbf, 0x1a, 0x7c, 0xfe, 0xc6, 0x8a, 0xd6, 0x32, 0xf6, 0x89, 0x15, 0x4f,
		  0x4f, 0x83, 0xe1, 0x3f, 0x89, 0x0d, 0x61, 0x60, 0x6a, 0x00, 0x6a, 0x00,
		  0xff, 0x76, 0x0a, 0xff, 0x76, 0x08, 0x6a, 0x00, 0x68, 0x00, 0x7c, 0x6a,
		  0x01, 0x6a, 0x10, 0xb4, 0x43, 0xb2, 0x80, 0x8b, 0xf4, 0xcd, 0x13, 0x61,
		  0x61, 0x8b, 0xf5, 0xea, 0x00, 0x7c, 0x00, 0x00, 0x45, 0x72, 0x72, 0x6f,
		  0x72, 0x21, 0x00, 0x44, 0x32, 0x31, 0x30, 0x41, 0x36, 0x31, 0x35, 0x2d,
		  0x41, 0x43, 0x46, 0x44, 0x2d, 0x34, 0x31, 0x34, 0x61, 0x2d, 0x42, 0x44,
		  0x46, 0x31, 0x2d, 0x46, 0x43, 0x39, 0x46, 0x32, 0x41, 0x38, 0x35, 0x46,
		  0x30, 0x37, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe8, 0x1b, 0xe6, 0x01,
		  0x00, 0x00, 0x00, 0x20, 0x21, 0x00, 0x0b, 0x4b, 0x81, 0x0a, 0x00, 0x08,
		  0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x4b, 0x82, 0x0a, 0x83, 0xd5,
		  0x89, 0x8c, 0x00, 0x08, 0x80, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0xd5,
		  0x8a, 0x8c, 0x83, 0x60, 0xd1, 0x0f, 0x00, 0x08, 0xa0, 0x00, 0x00, 0x00,
		  0x20, 0x00, 0x00, 0x60, 0xd2, 0x0f, 0x83, 0xed, 0xe8, 0xc5, 0x00, 0x08,
		  0xc0, 0x00, 0x00, 0xc0, 0x2c, 0x00, 0x55, 0xaa
};

int SCSIDeviceClass::readData(uint8_t* &data){
	//lcdConsole.println("readData:"+String(transferLength));

	data = transferData;

	if (dataSource == SCSIDEVICE_DATASOURCE_INTERNAL)
		return transferLength;

	int cnt = txLBAcnt - LBAcnt;
	if (cnt > maxTransferLBAcount) cnt = maxTransferLBAcount;
	//lcdConsole.println("txLBAcnt:"+String(txLBAcnt) + " LBAcnt:"+String(LBAcnt));
	//lcdConsole.println("cnt:"+String(cnt));

	int rl = 0;
	while ( cnt > 0 ) {
		//debugPrintln("readBlk:"+String(LBA) + " to:"+String(rl));
		//debug+="sdCard->readBlock("+String(LBA)+")\n";
		uint8_t r = sdCard->readBlock(LBA, transferData+rl); //r=0 failure r=1 success
		//debug+="sdCard->readBlock r:"+String(r)+"\n";

		if (r==0) {
			debugPrintln("r:"+String(r));
			scsiStatus = CHECK_CONDITION;
			senseKey = MEDIUM_ERROR; // or NOT_READY ?
			additionalSenseCode = NO_ASC;
			additionalSenseCodeQualifier = NO_ASCQ;
			return FAILURE;
		}

		cnt--;
		LBAcnt++;
		LBA++;
		rl += blockSize;
	}

	return rl;
}

/*
 * prepares the SCSIDeviceClass instance for read, sets LBA and size.
 * Actual read happens later by calling readData() from MSCDeviceClass instance
 */
int SCSIDeviceClass::handleRead10(SCSI_CBD_READ_10 &cbd, uint32_t len) {
	requestInfo+=" processRead10";

	//print(requestInfo);
	//println(" len"+String(len));

	txLBA = toUint32(cbd.LBA_a);
	//println(" txLBA"+String(txLBA,16));

	txLBAcnt = toUint16(cbd.transfer_length_a);
	//println(" txLBAcnt"+String(txLBAcnt,16));


	txLen = txLBAcnt * blockSize;
	LBA = txLBA;
	LBAcnt = 0;
	dataSource = SCSIDEVICE_DATASOURCE_SDCARD;
	transferLength = txLen;

	//debug+=" read LBA:"+String(LBA)+" txlen:"+String(txlen)+"\n";
	//lcdConsole.println("Read lba:"+String(LBA) +"ul:"+String(txLen));
	requestInfo+=" LBA:"+String(LBA)+" cnt:"+String(txLBAcnt);

	if (LBA+txLBAcnt-1 > lastLBA) {
		senseKey = ILLEGAL_REQUEST;
		additionalSenseCode = LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		additionalSenseCodeQualifier = ASCQ_READ_BOUNDARY_VIOLATION;
		uint32_t lba = LBA+txLBAcnt;
		memcpy(senseInformation,&lba,4);
		incorrectLengthIndicator=true;
		requestInfo+=" LBA OUT_OF_RANGE";
		return FAILURE;
	}
	//println(requestInfo);
	//if (cbd.transfer_length_a == 0) len=0;
	requestInfo+=" GOOD";
	return txLen;
}

int SCSIDeviceClass::writeData(uint8_t* &data){
	//println(requestInfo+" writeData");

	data = transferData;

	int cnt = txLBAcnt - LBAcnt;
	if (cnt > maxTransferLBAcount) cnt = maxTransferLBAcount;

	int wl = 0;
	while ( cnt > 0 ) {

		if (LBA+txLBAcnt-1 > lastLBA) {
			senseKey = ILLEGAL_REQUEST;
			additionalSenseCode = LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			additionalSenseCodeQualifier = ASCQ_READ_BOUNDARY_VIOLATION;
			uint32_t lba = LBA+txLBAcnt;
			memcpy(senseInformation,&lba,4);
			incorrectLengthIndicator=true;
			requestInfo+=" LBA OUT_OF_RANGE";
			println(" LBA OUT_OF_RANGE");
			return FAILURE;
		}

		uint8_t r = sdCard->writeBlock(LBA, data+wl);
		//uint8_t r = 1; //fake write
		//println(" writeBlock LBA:"+String(LBA)+" wl:"+String(wl));

		if (r==0) {
			scsiStatus = CHECK_CONDITION;
			senseKey = MEDIUM_ERROR; // or NOT_READY ?
			additionalSenseCode = NO_ASC;
			additionalSenseCodeQualifier = NO_ASCQ;
			println(" MEDIUM_ERROR");
			return FAILURE;
		}

		cnt--;
		LBAcnt++;
		LBA++;
		wl += blockSize;
	}

	return wl;
}

int SCSIDeviceClass::handleWrite10(SCSI_CBD_WRITE_10  &cbd, uint32_t len) {
	requestInfo="processWrite10";

	if (isWriteProtected) {
		scsiStatus = CHECK_CONDITION;
		senseKey = DATA_PROTECT; // or NOT_READY ?
		additionalSenseCode = ASC_WRITE_PROTECTED;
		additionalSenseCodeQualifier = ASCQ_WRITE_PROTECTED;
		println(" WRITE_PROTECTED");
		return FAILURE;
	}

	txLBA = toUint32(cbd.LBA_a);
	//println(" txLBA"+String(txLBA,16));

	txLBAcnt = toUint16(cbd.transfer_length_a);
	//println(" txLBAcnt"+String(txLBAcnt,16));

	txLen = txLBAcnt * blockSize;
	LBA = txLBA;
	LBAcnt = 0;
	dataSource = SCSIDEVICE_DATASOURCE_SDCARD;
	transferLength = txLen;

	//debug+=" read LBA:"+String(LBA)+" txlen:"+String(txlen)+"\n";
	//lcdConsole.println("Read lba:"+String(LBA) +"ul:"+String(txLen));
	requestInfo+=" LBA:"+String(LBA)+" cnt:"+String(txLBAcnt);

	if (LBA+txLBAcnt-1 > lastLBA) {
		senseKey = ILLEGAL_REQUEST;
		additionalSenseCode = LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		additionalSenseCodeQualifier = ASCQ_READ_BOUNDARY_VIOLATION;
		uint32_t lba = LBA+txLBAcnt;
		memcpy(senseInformation,&lba,4);
		incorrectLengthIndicator=true;
		requestInfo+=" LBA OUT_OF_RANGE";
		println(" LBA OUT_OF_RANGE");
		return FAILURE;
	}
	//if (cbd.transfer_length_a == 0) len=0;
	requestInfo+=" GOOD";

	//println(requestInfo); requestInfo="";

	return txLen;

}

int SCSIDeviceClass::handleRequestReadFormatCapacities(SCSI_CBD_READ_FORMAT_CAPACITIES &cbd, uint32_t len){
	debugPrintln("processRequestReadFormatCapacities:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	requestInfo="READ_FORMAT_CAPACITIES";
	scsiStatus = GOOD;

	uint16_t allocLen;
	msb2lsb(cbd.allocation_length, allocLen);
	debugPrintln("allocLen:"+String(allocLen));

	/* this is hex dump of a good dev
	 *     r1 r2 r3 CLH  NUM OF BLKS	r
	0000   00 00 00 10   00 ec e0 00   02 00 02 00
	                     00 ec e0 00   00 00 02 00
	*/
	SCSI_CBD_READ_FORMAT_CAPACITIES_DATA readFormatCapacitiesData;

	memset(&readFormatCapacitiesData, 0,sizeof(readFormatCapacitiesData));

	readFormatCapacitiesData.capacity_list_header.capacity_list_length = 16;

	uint32_t numOfBlks = SDCardSize();
	uint8_t blockLen[3] = {0x00, 0x02, 0x00}; // 512

	msb2lsb(numOfBlks,
			readFormatCapacitiesData.maximum_capacity_descritpor.numer_of_blocks);

	memcpy(readFormatCapacitiesData.maximum_capacity_descritpor.block_length, blockLen, 3);
	readFormatCapacitiesData.maximum_capacity_descritpor.descritpor_code
		= FORMAT_CAPACITY_DESCRIPTOR_CODE_FORMATTED_MEDIA_CUR;

	FORMAT_CAPACITY_DESCRIPTOR fcd;
	msb2lsb(numOfBlks, fcd.numer_of_blocks);
	memcpy(fcd.block_length, blockLen, 3);
	fcd.descritpor_code = 0x00;
	readFormatCapacitiesData.formattable_capacity_descritpors[0] = fcd;

	size_t tl = sizeof(readFormatCapacitiesData);

	debugPrintln("sizeof(readFormatCapacitiesData):"+String(tl));

	if (tl>allocLen) tl=allocLen;
	memcpy(transferData, &readFormatCapacitiesData, tl);
	transferLength = tl;
	return tl;
}

int SCSIDeviceClass::handleStartStop(SCSI_CBD_START_STOP  &cbd, uint32_t len){
	//lcdConsole.println("ModeSense:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	requestInfo+=" cbd.pwr_condition:x"+String(cbd.pwr_condition,16);
	requestInfo+=" cbd.LOEJ:"+String(cbd.LOEJ);
	requestInfo+=" cbd.start:"+String(cbd.start);

	if (cbd.pwr_condition == 0) { //START_VALID - Process the START and LOEJ bits.
		if (cbd.LOEJ && !cbd.start) {
			isSdCardReady = false;
		}
		if (cbd.LOEJ && cbd.start) {
			initSD();
		}
	}

	transferLength = 0;
	requestInfo+=" OK";
	return 0;
}

uint8_t SCSIDeviceClass::getMSCResultCase(){
	return MSCResultCase;
}

int SCSIDeviceClass::isRequestMeaningful(SCSI_CBD &cbd, uint32_t len, uint8_t msc_direction){
	if (cbd.generic.opcode == SCSI_TEST_UNIT_READY && len == 0){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_READ_10 && msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_WRITE_10 && msc_direction == 0){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL && len == 0){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_INQUIRY	&& len == 36 && msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_READ_CAPACITY_10 && len == 8 && msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_MODE_SENSE_6
			&& len == sizeof(SCSI_CBD_MODE_SENSE_DATA_6) && msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_REQUEST_SENSE
			&& len == sizeof(SCSI_CBD_REQUEST_SENSE_DATA) && msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_READ_FORMAT_CAPACITIES
			&& len == sizeof(SCSI_CBD_READ_FORMAT_CAPACITIES_DATA)
			&& msc_direction == 1){
		return GOOD;
	};
	if (cbd.generic.opcode == SCSI_START_STOP && len == 0){
		return GOOD;
	};

	scsiStatus = CHECK_CONDITION;
	senseKey = ILLEGAL_REQUEST;
	additionalSenseCode = INVALID_COMMAND_OPERATION_CODE;
	return FAILURE;
}

int SCSIDeviceClass::handleRequest(SCSI_CBD &cbd, uint32_t len){
	scsiStatus = GOOD;
	senseKey = NO_SENSE;
	additionalSenseCode = NO_ASC;
	additionalSenseCodeQualifier = NO_ASCQ;
	incorrectLengthIndicator = false;

	requestInfo=String(millis())+":";

	debugPrintln("rqOpCd:"+String(cbd.generic.opcode,16));

	if (cbd.generic.opcode == SCSI_TEST_UNIT_READY){
		requestInfo+="TEST_UNIT_READY";
		int r = handleTestUnitReady(cbd.unit_ready, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_10){
		requestInfo+="READ";
		int r = handleRead10(cbd.read10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_WRITE_10){
		requestInfo+="WRITE";
		int r = handleWrite10(cbd.write10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL){
		requestInfo+="PREVENT_ALLOW_MEDIUM_REMOVAL";
		int r = handleMediumRemoval(cbd.medium_removal, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_INQUIRY){
		requestInfo+="INQUIRY";
		int r = handleInquiry(cbd.inquiry, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_CAPACITY_10){
		requestInfo+="READ_CAPACITY";
		int r = handleReadCapacity10(cbd.read_capacity10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_MODE_SENSE_6){
		requestInfo+="MODE_SENSE";
		int r = handleModeSense6(cbd.mode_sense6, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_REQUEST_SENSE){
		requestInfo+="REQUEST_SENSE";
		int r = handleRequestSense(cbd.request_sense, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_FORMAT_CAPACITIES){
		requestInfo+="READ_FORMAT_CAPACITIES";
		int r = handleRequestReadFormatCapacities(cbd.request_read_format_capacities, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_START_STOP){
		requestInfo+="SCSI_START_STOP";
		int r = handleStartStop(cbd.start_stop, len);
		return r;
	};

	//lcdConsole.println("UNSUPPORTED:"+String(cbd.generic.opcode));

	requestInfo+="ILLEGAL_REQUEST:"+String(cbd.generic.opcode,16)+"h "+String(millis())+"ms";
	scsiStatus = CHECK_CONDITION;
	senseKey = ILLEGAL_REQUEST;
	additionalSenseCode = INVALID_COMMAND_OPERATION_CODE;
	return FAILURE;
}


