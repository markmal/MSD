/*
 * SCSIDevice.cpp
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */

#include "SCSIDevice.h"

#include <stdint.h>
#include "my_debug.h"
#include "SCSI.h"
#include "MSC.h"
//#include <SPI.h>
//#include <SD.h>
//#include <utility/Sd2Card.h>
//#include <utility/Sd2File.h>
#include "LcdConsole.h"

SCSIDevice::SCSIDevice():
	lastLBA(0), LBA(0), blockSize(SD_BLOCK_SIZE), sdCardErrorCode(0), sdCardErrorData(0){
	//sdCard = NULL;

	transferLength = 0;
	txLBA=0;
	txLBAcnt=0;
	txLen=0;
	LBAcnt=0;

	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	maxTransferLength = MAX_TRANSFER_LENGTH;
	maxTransferLBAcount = maxTransferLength / blockSize;
	transferData = (uint8_t*)malloc(MAX_TRANSFER_LENGTH);
	memset(transferData, 0, MAX_TRANSFER_LENGTH);
	memset(inquiryData.array, 0, sizeof(inquiryData));
	memset(capacity10.array , 0, sizeof(capacity10));

	sdCard = new Sd2Card();
	//initSD();
}

SCSIDevice::~SCSIDevice() {
	// TODO Auto-generated destructor stub
	delete sdCard;
	free(transferData);
}

int SCSIDevice::processInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len) {
	//lcdConsole.println("Inquiry:"+ String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	memset(&inquiryData, 0, sizeof(inquiryData));
	inquiryData.inquiry.peripheral_device_type = SCSI_SBC2;
	inquiryData.inquiry.RMB = 1;
	inquiryData.inquiry.version = SCSI_SPC2; // SPC-2
	inquiryData.inquiry.additional_length = 32;
	uint8_t t10_vendor_id[8] = {'M','M','7',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.t10_vendor_id, t10_vendor_id, 8);
	uint8_t product_id[16] = {'S','V','M','L',' ','A','F','M','0',' ',' ',' ',' ',' ',' ',' '};
	memcpy(inquiryData.inquiry.product_id, product_id, 16);
	uint8_t product_revision_level[4] = {'0','0','0','1'};
	memcpy(inquiryData.inquiry.product_revision_level, product_revision_level, 4);
	uint8_t drive_serial_number[8] = {'1','2','3','4','5','6','7','8'};
	memcpy(inquiryData.inquiry.drive_serial_number, drive_serial_number, 8);
	memcpy(transferData, inquiryData.array, 36);
	transferLength = 36;
	return 36;
}

uint8_t  SCSIDevice::SDCardType() {return sdCard->type();}
uint64_t SCSIDevice::SDCardSize() {return 512ull * sdCard->cardSize();}
String   SCSIDevice::SDCardProductName() {
	cid_t cid;
	sdCard->readCID(&cid);
	//lcdConsole.println("Manuf Id:"+String(cid.mid));
	return String(cid.pnm);
}

int SCSIDevice::begin(){
	return initSD();
}

int SCSIDevice::initSD(){
	if (!sdCard->init(SPI_FULL_SPEED, chipSelect)) {
		sdCardErrorCode = sdCard->errorCode();
		sdCardErrorData = sdCard->errorData();
		//lcdConsole.println("SD card initialization failed.");
		//lcdConsole.println("sdCardErrorCode:"+String(sdCardErrorCode));
		//lcdConsole.println("sdCardErrorData:"+String(sdCardErrorData));
		return -1;
	}
	return 0;
}


int SCSIDevice::processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint32_t len) {
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	//TODO Check if SD is ready
	///if (!sdCard) return NO_MEDIA;
	//if (!(sdCard->type() waitNotBusy(SD_INIT_TIMEOUT))) return MEDIA_BUSY;
	len = 0;
	transferLength = 0;
	return MEDIA_READY;
}

int SCSIDevice::processModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint32_t len) {
	//lcdConsole.println("ModeSense:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	if (cbd.PC == 0){ // current values
		if (cbd.page_code == 0x3F)
			if (cbd.subpage_code == 0x00 ) {
				modesenseData6.fields.mode_data_length = 3;
				modesenseData6.fields.medium_type = 0x00;
				modesenseData6.fields.dev_specific_param  = 0x00;
				modesenseData6.fields.block_descr_length = 0;
			}
	}
	memcpy(transferData, modesenseData6.array, 4);
	transferLength = 4;
	return 4;
}

int SCSIDevice::processMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint32_t len){
	//lcdConsole.println("MediumRemoval:"+String(len));
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	// if (sdCard in ) return 0
	// else return ERROR_NO_MEDIA
	transferLength = 0;
	return 0;
}

int SCSIDevice::processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint32_t len) {
	//lcdConsole.println("ReadCapacity10:"+ String(len));
	///uint32_t sz = sdCard->cardSize();
	///capacity10.fields.lastLBA  = sz - 1;
	dataSource = SCSIDEVICE_DATASOURCE_INTERNAL;
	lastLBA = 0xFFFFF;
	SCSI_CAPACITY_DATA_10 cd;
	cd.fields.lastLBA = lastLBA;
	cd.fields.block_sz = blockSize;

	msb2lsb(cd.fields.lastLBA, capacity10.fields.lastLBA);
	msb2lsb(cd.fields.block_sz, capacity10.fields.block_sz);

	memcpy(transferData, capacity10.array, 8);
	transferLength = 8;
	return 8;
}

uint16_t toUint16(uint8_t a[2]){
	return a[0]<<8 | a[1];
}
uint32_t toUint32(uint8_t a[4]){
	return a[0]<<24 | a[1]<<16 | a[2]<<8 | a[3];
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

int SCSIDevice::readData(uint8_t* &data){
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
		//lcdConsole.println("readBlock:"+String(LBA) + " to:"+String(rl));
		uint8_t r = sdCard->readBlock(LBA, transferData+rl);

		//if (r==0) error; TODO

		/* // spoof it
		if (LBA==0){
			memset(transferData, 0, maxTransferLength);
			memcpy(transferData, BLOCK0, 512);
		} else
			for(int i=0; i<blockSize; i++) {
				transferData[rl+i] = (i%16)+0x41;
			}
		*/

		cnt--;
		LBAcnt++;
		LBA++;
		rl += blockSize;
	}

	return rl;
}


int SCSIDevice::processRead10(SCSI_CBD_READ_10 &cbd, uint32_t len) {

	txLBA = toUint32(cbd.LBA_a);
	txLBAcnt = toUint16(cbd.length_a);
	txLen = txLBAcnt * blockSize;
	LBA = txLBA;
	LBAcnt = 0;
	dataSource = SCSIDEVICE_DATASOURCE_SDCARD;
	transferLength = txLen;

	//debug+=" read LBA:"+String(LBA)+" txlen:"+String(txlen)+"\n";
	//lcdConsole.println("Read lba:"+String(LBA) +"ul:"+String(txLen));

	//if (LBA > lastLBA) error();
	//if (cbd.length == 0) len=0;

	return txLen;

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


int SCSIDevice::processWrite10(SCSI_CBD_WRITE_10  &cbd, uint32_t len) {
	lcdConsole.println("Write10:"+ String(len));

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
	return len;
}


int SCSIDevice::processRequest(SCSI_CBD &cbd, uint32_t len){

	if (cbd.generic.opcode == SCSI_READ_10){
		int r = processRead10(cbd.read10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_WRITE_10){
		//debug += "  WRITE10\n";
		int r = processWrite10(cbd.write10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_TEST_UNIT_READY){
		//debug += "  TEST_UNIT_READY\n";
		int r = processTestUnitReady(cbd.unit_ready, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_INQUIRY){
		//debug += "  INQUIRY\n";
		int r = processInquiry(cbd.inquiry, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_READ_CAPACITY_10){
		//debug += "  READ_CAPACITY_10\n";
		int r = processReadCapacity10(cbd.read_capacity10, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_MODE_SENSE_6){
		//debug += "  MODE_SENSE_6\n";
		int r = processModeSense6(cbd.mode_sense6, len);
		return r;
	};
	if (cbd.generic.opcode == SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL){
		//debug += "  SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL\n";
		int r = processMediumRemoval(cbd.medium_removal, len);
		return r;
	};

	lcdConsole.println("UNSUPPORTED:"+String(cbd.generic.opcode));

	return SCSI_UNSUPPORTED_OPERATION;
}


