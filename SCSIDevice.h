/*
 * SCSIDevice.h
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */

#ifndef MSC__SCSIDevice_h
#define MSC__SCSIDevice_h

#include <stdint.h>
//#include <SPI.h>
//#include <SD.h>
#include "utility/Sd2Card.h"

#include "SCSI.h"

#define SD_BLOCK_SIZE 512
#define MAX_TRANSFER_LENGTH 4096
#define SCSIDEVICE_DATASOURCE_INTERNAL 0
#define SCSIDEVICE_DATASOURCE_SDCARD 1


class SCSIDevice {
public:
	SCSIDevice();
	virtual ~SCSIDevice();
	int begin();

	int initSD();

	int readData(uint8_t* &data);
	//from the device to the host
	int processRequest(SCSI_CBD &cbwcb, uint32_t len);
	//from the host to the device
	//int processOutRequest(SCSI_CBD &cbwcb,	uint8_t* &data, uint8_t& len);

	int processInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len);
	int processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint32_t len);
	int processModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint32_t len);
	int processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint32_t len);
	int processRead10(SCSI_CBD_READ_10  &cbd, uint32_t len);
	int processWrite10(SCSI_CBD_WRITE_10 &cbd, uint32_t len);
	int processMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint32_t len);

	uint8_t  SDCardType();
	uint64_t SDCardSize();
	String   SDCardProductName();

private:
	SCSI_STANDARD_INQUIRY_DATA inquiryData;
	SCSI_CAPACITY_DATA_10 capacity10;
	SCSI_CBD_MODE_SENSE_DATA_6 modesenseData6;

	uint32_t lastLBA;
	uint32_t LBA;
	uint32_t txLBA;
	uint32_t txLen;
	uint32_t transferLength;
	uint32_t maxTransferLength;
	uint32_t blockSize;

	Sd2Card* sdCard;
	uint8_t* transferData;

	const int chipSelect = 4;

	uint16_t txLBAcnt;
	uint16_t LBAcnt;
	uint16_t maxTransferLBAcount;

	uint8_t dataSource;
	uint8_t sdCardErrorCode;
	uint8_t sdCardErrorData;
};

#endif /* SCSIDevice_h */
