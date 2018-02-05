/*
 * SCSIDevice.h
 *
 *  Created on: Feb 1, 2018
 *      Author: mmalakanov
 */

#ifndef SRC_SCSIDEVICE_H_
#define SRC_SCSIDEVICE_H_

#include <stdint.h>
#include "SCSI.h"
#include "SD.h"
//#include "Sd2Card.h"

class SCSIDevice {
public:
	SCSIDevice();
	virtual ~SCSIDevice();

	//from the device to the host
	int processRequest(SCSI_CBD &cbwcb,	uint8_t* &data, uint8_t& len);
	//from the host to the device
	//int processOutRequest(SCSI_CBD &cbwcb,	uint8_t* &data, uint8_t& len);

	int processInquiry(SCSI_CBD_INQUIRY  &cbd, uint8_t* &data, uint8_t& len);
	int processTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint8_t* &data, uint8_t& len);
	int processReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint8_t* &data, uint8_t& len);
	int processRead10(SCSI_CBD_READ_10  &cbd, uint8_t* &data, uint8_t& len);
	int processWrite10(SCSI_CBD_WRITE_10  &cbd, uint8_t* &data, uint8_t& len);

private:
	uint8_t blockData[512];
	SCSI_STANDARD_INQUIRY_DATA inquiryData;
	SCSI_CAPACITY_DATA_10 capacity10;
	Sd2Card* sdCard;
	uint8_t LUN;
	uint8_t sdCardErrorCode;
	uint8_t sdCardErrorData;
};

#endif /* SRC_SCSIDEVICE_H_ */
