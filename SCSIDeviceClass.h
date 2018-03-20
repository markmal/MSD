/*
 * SCSIDeviceClass.h
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

#ifndef MSC__SCSIDevice_h
#define MSC__SCSIDevice_h

#define SCSI_DEVICE_CLASS_DEBUG
//#define SCSI_DEVICE_CLASS_LCD_DEBUG

#include <stdint.h>
//#include <SPI.h>
//#include <SD.h>
#include "my_debug.h"
#ifndef DUMMY_SD
#include "utility/Sd2Card.h"
#endif

#include "SCSI.h"

#define SD_BLOCK_SIZE 512
#define MAX_TRANSFER_LENGTH 4096
#define SCSIDEVICE_DATASOURCE_INTERNAL 0
#define SCSIDEVICE_DATASOURCE_SDCARD 1

// these will be converted in MSC into CSW_PASS, CSW_FAILURE, CSW_PHASE_ERROR
#define PASS 	 0
#define FAILURE -1
#define PHASE_ERROR -2

class SCSIDeviceClass {
public:
	SCSIDeviceClass();
	virtual ~SCSIDeviceClass();
	int begin();

	int initSD();

	//from the device to the host
	int readData(uint8_t* &data);

	//from the host to the device
	int writeData(uint8_t* &data);

	int handleRequest(SCSI_CBD &cbwcb, uint32_t len);

	int handleInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len);
	int handleStandardInquiry(SCSI_CBD_INQUIRY  &cbd, uint32_t len);
	int handleInquirySupportedVPDPages(SCSI_CBD_INQUIRY  &cbd, uint32_t len);
	int handleDeviceIdentification(SCSI_CBD_INQUIRY  &cbd, uint32_t len);
	int handleInquiryUnitSerialNumberPage(SCSI_CBD_INQUIRY  &cbd, uint32_t len);

	int handleTestUnitReady(SCSI_CBD_TEST_UNIT_READY  &cbd, uint32_t len);
	int handleRequestSense(SCSI_CBD_REQUEST_SENSE  &cbd, uint32_t len);
	int handleModeSense6(SCSI_CBD_MODE_SENSE_6  &cbd, uint32_t len);
	int handleReadCapacity10(SCSI_CBD_READ_CAPACITY_10  &cbd, uint32_t len);
	int handleRead10(SCSI_CBD_READ_10  &cbd, uint32_t len);
	int handleWrite10(SCSI_CBD_WRITE_10 &cbd, uint32_t len);
	int handleMediumRemoval(SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL &cbd, uint32_t len);
	int handleRequestReadFormatCapacities(SCSI_CBD_READ_FORMAT_CAPACITIES &cbd, uint32_t len);
	int handleStartStop(SCSI_CBD_START_STOP  &cbd, uint32_t len);

	uint32_t getMaxTransferLength();

	uint8_t  SDCardType();
	uint64_t SDCardSize();
	String   SDCardProductName();
	uint8_t  SDCardErrorCode();
	uint8_t  SDCardErrorData();
	String   getSDCardError();
	String   getSCSIError();
	uint8_t  getMSCResultCase();

	String requestInfo;

private:
	//SCSI_STANDARD_INQUIRY_DATA inquiryData;
	//SCSI_CAPACITY_DATA_10 capacity10;
	//SCSI_CBD_MODE_SENSE_DATA_6 modeSenseData6;
	//SCSI_CBD_REQUEST_SENSE_DATA requestSenseData;

	uint32_t lastLBA;
	uint32_t LBA;
	uint32_t txLBA;
	uint32_t txLen;
	uint32_t transferLength;
	uint32_t maxTransferLength;
	uint32_t blockSize;

#ifdef DUMMY_SD
	Sd2CardDummy* sdCard;
#else
	Sd2Card* sdCard;
#endif

	uint8_t* transferData;

	const int chipSelect = 4;

	uint16_t txLBAcnt;
	uint16_t LBAcnt;
	uint16_t maxTransferLBAcount;

	uint8_t MSCResultCase; // 13 Cases: 1 is Case1 etc...
	uint8_t senseKey;
	uint8_t dataSource;
	uint8_t sdCardErrorCode;
	uint8_t sdCardErrorData;

	uint8_t senseInformation[4];
	uint8_t additionalSenseCode;
	uint8_t additionalSenseCodeQualifier;
	bool incorrectLengthIndicator;
	bool isWriteProtected;
	bool isSdCardReady;
public:
	uint8_t scsiStatus;
	uint8_t align01;
};

#endif /* SCSIDevice_h */
