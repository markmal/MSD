/*
 * PluggableUSBMSCModule.h
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#ifndef MSC__MSC_h
#define MSC__MSC_h

#include <stdint.h>
#include <Arduino.h>
#include <USB/PluggableUSB.h>
#include "SCSI.h"
#include "SCSIDevice.h"

#define EPTYPE_DESCRIPTOR_SIZE		uint32_t
#define USB_SendControl				USBDevice.sendControl
#define USB_Available				USBDevice.available
#define USB_Recv					USBDevice.recv
#define USB_Send					USBDevice.send
#define USB_Flush					USBDevice.flush
#define is_write_enabled(x)			(1)


#define NUM_INTERFACE 1
#define NUM_ENDPOINTS 2
#define MSC_REPORT_PROTOCOL 0

#define MSC_ENDPOINT_OUT	pluggedEndpoint
#define MSC_ENDPOINT_IN	((uint8_t)(pluggedEndpoint+1))


#define MSC_SUBCLASS_NOTREPORTED 0x00
#define MSC_SUBCLASS_RBC 0x01
#define MSC_SUBCLASS_MMC5 0x02 // ATAPI
#define MSC_SUBCLASS_QIC 0x03 // obsolete
#define MSC_SUBCLASS_UFI 0x04 // Floppy
#define MSC_SUBCLASS_SFF 0x05 // obsolete
#define MSC_SUBCLASS_TRANSPARENT 0x06 // Allocated by USB-IF for SCSI. SCSI standards are defined outside of USB.
#define MSC_SUBCLASS_LSDFS 0x07 // LSDFS specifies how host has to negotiate access before tryingSCSI
#define MSC_SUBCLASS_IEEE 0x07 // Allocated by USB-IF for IEEE 1667. IEEE 1667 is defined outside of USB.
#define MSC_SUBCLASS_VENDORSPECIFIC 0xFF

#define MSC_REQUEST_STATUS   		0x00
#define MSC_REQUEST_GET_REQUEST   	0xFC
#define MSC_REQUEST_PUT_REQUEST   	0xFD
#define MSC_REQUEST_GET_MAX_LUN   	0xFE //Assigned by USB Mass Storage Class Bulk-Only (BBB) Transport
#define MSC_REQUEST_BOMSR   		0xFF //Assigned by USB Mass Storage Class Bulk-Only (BBB) Transport

#define MSC_GET_REPORT   0x01
#define MSC_GET_PROTOCOL 0x02
#define MSC_GET_IDLE     0x03
#define MSC_SET_REPORT   0x04
#define MSC_SET_PROTOCOL 0x05
#define MSC_SET_IDLE     0x06

typedef struct
{
  uint8_t len;      // 9
  uint8_t dtype;    // 0x21
  uint8_t addr;
  uint8_t versionL; // 0x101
  uint8_t versionH; // 0x101
  uint8_t country;
  uint8_t desctype; // 0x22 report
  uint8_t descLenL;
  uint8_t descLenH;
} MSCDescDescriptor;

/*
class MSCSubDescriptor {
public:
	MSCSubDescriptor *next; //= NULL;
	MSCSubDescriptor(const void *d, const uint16_t l) : data(d), length(l), next(NULL) { }
  const void* data;
  const uint16_t length;
};
*/

#define le32_t uint32_t

/**
 * Command Block Wrapper (CBW).
 */
struct USB_MSC_CBW {
	le32_t dCBWSignature;	//!< Must contain 'USBC'
	le32_t dCBWTag;	//!< Unique command ID
	le32_t dCBWDataTransferLength;	//!< Number of bytes to transfer
	uint8_t bmCBWFlags;	//!< Direction in bit 7
	uint8_t bCBWLUN;	//!< Logical Unit Number
	uint8_t bCBWCBLength;	//!< Number of valid CBWCB bytes
	uint8_t CBWCB[16];	//!< SCSI Command Descriptor Block
	uint8_t alignTo32b;
};

#define  USB_CBW_SIZE          		31	//!< CBW size
#define  USB_CBW_SIGNATURE          0x43425355 //!< dCBWSignature value, LE
//#define  USB_CBW_SIGNATURE          0x55534243	//!< dCBWSignature value
#define  USB_CBW_DIRECTION_IN       (1<<7)	//!< Data from device to host
#define  USB_CBW_DIRECTION_OUT      (0<<7)	//!< Data from host to device
#define  USB_CBW_LUN_MASK           0x0F	//!< Valid bits in bCBWLUN
#define  USB_CBW_LEN_MASK           0x1F	//!< Valid bits in bCBWCBLength

/**
 * Command Status Wrapper (CSW).
 */
struct USB_MSC_CSW {
	le32_t dCSWSignature;	//!< Must contain 'USBS'
	le32_t dCSWTag;	//!< Same as dCBWTag
	le32_t dCSWDataResidue;	//!< Number of bytes not transfered
	uint8_t bCSWStatus;	//!< Status code
};

#define  USB_CSW_SIZE          		13	//!< CSW size
//#define  USB_CSW_SIGNATURE          0x55534253	//!< dCSWSignature value BE
#define  USB_CSW_SIGNATURE          0x53425355	//!< dCSWSignature value LE
#define  USB_CSW_STATUS_PASS        0x00	//!< Command Passed
#define  USB_CSW_STATUS_FAIL        0x01	//!< Command Failed
#define  USB_CSW_STATUS_PE          0x02	//!< Phase Error



#define MSC_BLOCK_DATA_SZ 512
//#define MSC_BLOCK_DATA_SZ 64
//extern byte blockData[BLOCK_DATA_SZ];
//extern String debug;

/* The PluggableUSBModule must implement setup, getInterface and getDescriptor functions
 * and declare how many endpoints and interfaces it needs to allocate
 */


class MSC_: public PluggableUSBModule {

public:
	MSC_();
	virtual ~MSC_();
	int begin();
	uint32_t receiveBlock(); // receives block from USB
	//uint32_t sendBlock(); // sends block to USB
	//bool begin();
protected:
  // Implementation of the PluggableUSBModule
  int getInterface(uint8_t* interfaceCount);
  int getDescriptor(USBSetup& setup);
  bool setup(USBSetup& setup);
  uint8_t getShortName(char* name);
  bool reset();

  uint32_t receiveInBlock(); // receives IN block from USB
  uint32_t receiveOutBlock(); // receives OUT block from USB

private:
  USB_MSC_CBW cbw;
  USB_MSC_CSW csw;
  SCSIDevice scsiDev;
  uint32_t txEndpoint;
  uint32_t rxEndpoint;
  EPTYPE_DESCRIPTOR_SIZE epType[NUM_ENDPOINTS];   ///< Container that defines the two bulk MIDI IN/OUT endpoints types
  //uint8_t response[4096];
  uint8_t* data; // = (uint8_t*)response;

  //const uint32_t *endpointType;
  //uint16_t descriptorSize;
  //uint8_t pluggedInterface;
  //uint8_t pluggedEndpoint;
  //const uint8_t numEndpoints;
  //const uint8_t numInterfaces;
  //uint8_t protocol;
  //uint8_t idle;

};

//extern MSC_& MSC();
//extern MSC_ MSC;

#endif /* MSC_h_ */
