/*
 * SCSIDevice.h
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */

#ifndef MSC__SCSIDevice_h
#define MSC__SCSIDevice_h

#include <stdint.h>
//#include "SD.h"
//#include "utility/Sd2Card.h"
//#include <SPI.h>

#include "SCSI.h"

#define SD_BLOCK_SIZE 512


class SCSIDevice {
public:
	SCSIDevice();
	virtual ~SCSIDevice();

	//from the device to the host
	int processRequest(SCSI_CBD &cbwcb, uint8_t* &data, uint16_t& len);
	//from the host to the device
	//int processOutRequest(SCSI_CBD &cbwcb,	uint8_t* &data, uint8_t& len);

	int processInquiry(SCSI_CBD_INQUIRY  &cbd, uint8_t* &data, uint16_t& len);
	int processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint8_t* &data, uint16_t& len);
	int processModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint8_t* &data, uint16_t& len);
	int processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint8_t* &data, uint16_t& len);
	int processRead10(SCSI_CBD_READ_10  &cbd, uint8_t* &data, uint16_t& len);
	int processWrite10(SCSI_CBD_WRITE_10 &cbd, uint8_t* &data, uint16_t& len);
	int processMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint8_t* &data, uint16_t& len);

private:
	SCSI_STANDARD_INQUIRY_DATA inquiryData;
	SCSI_CAPACITY_DATA_10 capacity10;
	SCSI_CBD_MODE_SENSE_DATA_6 modesenseData6;
	//Sd2Card* sdCard;
	uint32_t lastLBA;
	uint32_t blockSize;
	uint8_t blockData[];
	uint8_t sdCardErrorCode;
	uint8_t sdCardErrorData;
};

#endif /* SCSIDevice_h */
