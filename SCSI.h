/*
 * SCSI.h
 *
 *  Created on: Jan 29, 2018
 *      Author: mark
 */

#ifndef MSC__SCSI_h
#define MSC__SCSI_h

#include <stdint.h>
#include "my_debug.h"

// SCSI commands ref: https://www.seagate.com/staticfiles/support/disc/manuals/scsi/100293068a.pdf

// opcodes required (by Windows) for USB card readers -> https://blogs.msdn.microsoft.com/usbcoreblog/2011/07/21/usb-mass-storage-and-compliance/
#define SCSI_FORMAT UNIT					0x04
#define SCSI_INQUIRY 						0x12
#define SCSI_MODE_SELECT_6 					0x15
#define SCSI_MODE_SENSE_6 					0x1A
#define SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1E
#define SCSI_READ_CAPACITY_10 				0x25
#define SCSI_READ_CAPACITY_16 				0x9E // really ?
#define SCSI_READ_FORMAT_CAPACITIES			0x23 // MMC2, requested by Windows
#define SCSI_READ_10 						0x28
#define SCSI_RECEIVE_DIAGNOSTIC_RESULTS		0x1C // when ENCSERV=1 ???
#define SCSI_REPORT LUNS					0xA0
#define SCSI_REQUEST_SENSE 					0x03
#define SCSI_START_STOP 					0x1B
#define SCSI_TEST_UNIT_READY 				0x00
#define SCSI_WRITE_10 						0x2A
//#define SCSI_VERIFY 						0x2F

struct SCSI_CDB_CONTROL { // Generic just get opcode
    uint8_t	LINK:1, obsolete:1, NACA:1, reserv:3, vendorspcfc:2;
};

struct SCSI_CBD_GENERIC { // Generic just get opcode
    uint8_t	opcode;
	uint8_t array[15];
};

struct SCSI_CBD_INQUIRY {
	  uint8_t	opcode; //
	  uint8_t	EVPD:1, CMDDT:1, reserv:6;
	  uint8_t	pgcode;
	  uint16_t	allocation_length;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[10];
};

//#define INQUIRY_RMB_NOTREMOVABLE 0<<7
//#define INQUIRY_RMB_REMOVABLE 1<<7

//PERIPHERAL DEVICE TYPE
#define SCSI_SBC2	0x00 // Direct access block device (e.g., magnetic disk)
#define SCSI_RBC	0x0E // Simplified direct-access device (e.g., magnetic disk) -- Windows does not support it
#define SCSI_SPC2	0x04 // The device complies to ANSI INCITS 351-2001 (SPC-2)

struct SCSI_STANDARD_INQUIRY_DATA{
		uint8_t peripheral_device_type:5, peripheral_qualifier:3;
		uint8_t reserv1:7, RMB:1; // = INQUIRY_RMB_REMOVABLE; // RMB(bit 7), resrved(0-6)
		uint8_t version; //=0;
		uint8_t response_data_format:4, HISUP:1, NORMACA:1, obsolete1:2;
		uint8_t additional_length; // N-4
		uint8_t protect:1, reserv2:2, b3PC:1, TPGS:2, ACC:1, SCCS:1;
		uint8_t adr16:1, obsolete2:2, MCHNGR:1, MULTIP:1, VS:1, ENSERV:1, BQUE:1;
		uint8_t VS2:1, CMDQUE:1, obsolete3:1, linked:1, SYNC:1, WBUS16:1, obsolete4:2;

		uint8_t t10_vendor_id[8]; //={'M','M','7',' ',' ',' ',' ',' '};
		uint8_t product_id[16]; //={'S','V','M','L',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
		uint8_t product_revision_level[4]; //={'0','0','0','1'};
		// 36 bytes

		uint8_t drive_serial_number[8]; //={'0','0','0','1','0','0','0','1'};
		uint8_t vendor_unique[12]; //={'0','0','0','1','0','0','0','1'};
		uint8_t IUS:1, QAS:1, clocking:2, reserv3:4;

		uint8_t alignTo60[3];
};
extern SCSI_STANDARD_INQUIRY_DATA standardInquiry;

/*
struct SCSI_CBD_READ_FORMAT_CAPACITY {
	  uint8_t	opcode;
	  uint8_t   rest[15];
};
*/

struct SCSI_CBD_READ_CAPACITY_10 {
	  uint8_t	opcode; // 0x25
	  uint8_t	reserv1;
	  uint32_t	LBA;
	  uint8_t	reserv2[2];
	  uint8_t	PMI:1, reserv3:7;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[6];
};
/*
 * The LOGICAL BLOCK ADDRESS field shall be set to zero if the PMI bit is set to zero. If the PMI bit is set to zero and the
LOGICAL BLOCK ADDRESS field is not set to zero, the device server shall terminate the command with CHECK CONDI-
TION status with the sense key set to ILLEGAL REQUEST and the additional sense code set to INVALID FIELD IN CDB.

A partial medium indicator (PMI) bit set to zero specifies that the device server return information on the last logical
block on the direct-access block device

A PMI bit set to one specifies that the device server return information on the last logical block after that specified in
the LOGICAL BLOCK ADDRESS field before a substantial vendor-specific delay in data transfer may be encountered.
 */

struct SCSI_CAPACITY_DATA_10 {
	  uint32_t	lastLBA;
	  uint32_t	block_sz; // in bytes
};

struct SCSI_CBD_READ_CAPACITY_16 {
	  uint8_t	opcode; // 0x9E
	  uint8_t	srvact:5, reserv1:3;
	  uint64_t	lastLBA;
	  uint32_t	block_sz;
	  uint8_t	PMI:1, reserv2:7;
	  SCSI_CDB_CONTROL   control;
};

struct SCSI_CAPACITY_DATA_16 {
	  uint64_t	lastLBA;
	  uint32_t	block_sz; // in bytes
	  uint8_t PROT_EN:1, P_TYPE:3, reserv1:4;
	  uint8_t reserv2[19];
};


struct SCSI_CBD_READ_10 {
	  uint8_t	opcode; // 0x28
	  uint8_t	obsolete:1, FUA_NV:1, reserv1:1, FUA:1, DPO:1, RDPROTECT:3;
	  uint8_t	LBA_a[4]; // ! Do not use uint32_t ! the array is here for proper alignment!
	  uint8_t	GROUP_NO:5, reserv2:3;
	  uint8_t	transfer_length_a[2]; // ! Do not use uint16_t ! the array is here for proper alignment!
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[6];
};

struct SCSI_CBD_WRITE_10 {
	  uint8_t	opcode; // 0x2A
	  uint8_t	obsolete:1, FUA_NV:1, reserv1:1, FUA:1, DPO:1, RDPROTECT:3;
	  uint8_t	LBA_a[4]; // ! Do not use uint32_t ! the array is here for proper alignment!
	  uint8_t	GROUP_NO:5, reserv2:3;
	  uint8_t	transfer_length_a[2]; // ! Do not use uint16_t ! the array is here for proper alignment!
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[6];
};

struct SCSI_CBD_REQUEST_SENSE {
	  uint8_t	opcode; // 0x3
	  uint8_t	desc:1, reserv1:7;
	  uint8_t	reserv2[2];
	  uint8_t	allocation_length;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[10];
};

struct SCSI_CBD_REQUEST_SENSE_DATA {
	  uint8_t	response_code:7, valid:1;
	  uint8_t	obolete;
	  uint8_t	sense_key:4, reserv1:1, ILI:1, EOM:1, filemark:1;
	  uint8_t	information[4];
	  uint8_t	additional_sense_len;
	  uint8_t	command_information[4];
	  uint8_t	additional_sense_code;
	  uint8_t	additional_sense_code_qualifier;
	  uint8_t	field_replaceable_unit_code;
	  uint16_t	sense_key_specific:15, SKSV:1;
	  uint8_t	additional_sense_bytes;
};

struct SCSI_CBD_MODE_SENSE_6 {
	  uint8_t	opcode; // 0x1A
	  uint8_t	reserv1:3, DBD:1, reserv2:4;
	  uint8_t	page_code:6, PC:2; //for Mode Sense. Page Code
	  uint8_t	subpage_code;
	  uint8_t	allocation_length;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[10];
};

struct SCSI_CBD_MODE_SENSE_DATA_6 {
  	  uint8_t	mode_data_length;
	  uint8_t	medium_type;
	  uint8_t	dev_specific_param;
	  uint8_t	block_descr_length;
};


struct SCSI_CBD_TEST_UNIT_READY {
	  uint8_t	opcode; // 0x00
	  uint8_t	reserv[4];
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[10];
};


/*
struct SCSI_CBD_VERIFY {
	  uint8_t	opcode;
	  uint8_t	reserv1[4];
	  uint8_t	length; //alloc lenght or "Prevent Allow Flag" one bit 0 for Allow Media Removal opcode
	  uint8_t   rest[10];
};
*/

struct SCSI_CBD_START_STOP {
	  uint8_t	opcode; // 0x1B
	  uint8_t	immed:1, reserv1:7;
	  uint8_t	reserv2[2];
	  uint8_t	start:1, LOEJ:1, reserv3:2, pwr_condition:4;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[10];
};

struct SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL {
	  uint8_t	opcode; // 0x1E
	  uint8_t	reserv1:5, LUN:3; // LUN is obsolete
	  uint8_t	reserv2[2];
	  uint8_t	prevent:2, reserv3:6; //
	  uint8_t	link:1, flag:1, reserv4:4, vndr_specif:1, lock_shared:1;
	  uint8_t   rest[10];
};

struct SCSI_CBD_READ_FORMAT_CAPACITIES {
	  uint8_t	opcode; // 0x23
	  uint8_t	reserv1[6];
	  uint16_t	allocation_length;
	  SCSI_CDB_CONTROL   control;
	  uint8_t   rest[6];
};

struct SCSI_CBD_READ_FORMAT_CAPACITIES_DATA {
	struct CAPACITY_LIST_HEADER {
		uint8_t reserv1;
		uint8_t reserv2;
		uint8_t reserv3;
		uint8_t capacity_list_length;
	} capacity_list_header;

	struct CURRENT_CAPACITY_DESCRIPTOR {
		uint32_t numer_of_blocks;
		uint8_t descritpor_type:2, reserv1:6;
		uint8_t block_length[3];
	} current_capacity_descritpor;
};

union SCSI_CBD {
	SCSI_CBD_GENERIC generic;
	SCSI_CBD_INQUIRY inquiry;
	SCSI_CBD_READ_CAPACITY_10 read_capacity10;
	SCSI_CBD_READ_CAPACITY_16 read_capacity16;
	SCSI_CBD_READ_10 read10;
	SCSI_CBD_WRITE_10 write10;
	SCSI_CBD_REQUEST_SENSE request_sense;
	SCSI_CBD_MODE_SENSE_6 mode_sense6;
	SCSI_CBD_MODE_SENSE_DATA_6 sense_data6;
	SCSI_CBD_TEST_UNIT_READY unit_ready;
	SCSI_CBD_START_STOP start_stop;
	SCSI_CBD_PREVENT_ALLOW_MEDIUM_REMOVAL medium_removal;
	SCSI_CBD_READ_FORMAT_CAPACITIES  request_read_format_capacities;
	uint8_t array[16];
};

#define SCSI_UNSUPPORTED_OPERATION	-1

#define MEDIA_READY					0
#define ERROR_MEDIA_READ 			-2
#define ERROR_NO_MEDIA				-3
#define ERROR_MEDIA_BUSY			-4

// for response sense
#define CURRENT_ERRORS 		0x70
#define DEFERRED_ERRORS		0x71

// from linux scsi.h
// Status codes
#define GOOD                 0x00
#define CHECK_CONDITION      0x01
#define CONDITION_GOOD       0x02
#define BUSY                 0x04
#define INTERMEDIATE_GOOD    0x08
#define INTERMEDIATE_C_GOOD  0x0a
#define RESERVATION_CONFLICT 0x0c
#define COMMAND_TERMINATED   0x11
#define QUEUE_FULL           0x14

// Sense keys
#define NO_SENSE            0x00
#define RECOVERED_ERROR     0x01
#define NOT_READY           0x02
#define MEDIUM_ERROR        0x03
#define HARDWARE_ERROR      0x04
#define ILLEGAL_REQUEST     0x05
#define UNIT_ATTENTION      0x06
#define DATA_PROTECT        0x07
#define BLANK_CHECK         0x08
#define COPY_ABORTED        0x0a
#define ABORTED_COMMAND     0x0b
#define VOLUME_OVERFLOW     0x0d
#define MISCOMPARE          0x0e


/*
// ADDITIONAL_SENSE_CODES
#define SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED
#define SCSI_ASC_LOGICAL_UNIT_DOES_NOT_RESPOND_TO_SELECTION
#define SCSI_ASC_MEDIUM_NOT_PRESENT
#define SCSI_ASC_LOGICAL_UNIT_NOT_READY_CAUSE_NOT_REPORTABLE
#define SCSI_ASC_LOGICAL_UNIT_IS_IN_PROCESS_OF_BECOMING_READY
#define SCSI_ASC_LOGICAL_UNIT_NOT_READY_INITIALIZING_COMMAND_REQUIRED
#define SCSI_ASC_LOGICAL_UNIT_NOT_READY_MANUAL_INTERVENTION_REQUIRED
#define SCSI_ASC_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS
*/
#define NO_ASC 0x00
#define INVALID_COMMAND_OPERATION_CODE 0x20
#define ASC_WRITE_PROTECTED 0x27
#define LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE 0x21
#define NOT_READY_TO_READY_CHANGE_MEDIUM_MAY_HAVE_CHANGED 0x28

// ASCQ
#define NO_ASCQ 0x00
#define ASCQ_READ_BOUNDARY_VIOLATION 			0x7
#define ASCQ_WRITE_PROTECTED 				0x00

/*
uint16_t msb2lsb(uint16_t v);
uint32_t msb2lsb(uint32_t v);
uint64_t msb2lsb(uint64_t v);
*/

/*
 * converts Most significant bytes to Least significant bytes. And vice versa,
 * s.b0 -> d.b1, s.b1 -> d.b0
 * msb2lsb( source, dest );
 */
void msb2lsb(uint16_t& s, uint16_t& d);
/*
 * converts Most significant bytes to Least significant bytes. And vice versa,
 * s.b0 -> d.b3, s.b1 -> d.b2, s.b2 -> d.b1, s.b3 -> d.b0
 * msb2lsb( source, dest );
 */
void msb2lsb(uint32_t& s, uint32_t& d);
/*
 * converts Most significant bytes to Least significant bytes. And vice versa,
 * msb2lsb( source, dest );
 */
void msb2lsb(uint64_t& s, uint64_t& d);


/*
 * returns uint16_t from MSB-LSB array
 */
uint16_t toUint16(uint8_t a[2]);

/*
 * returns uint32_t from MSB-LSB array
 */
uint32_t toUint32(uint8_t a[4]);

#endif /* MSC__SCSI_h */
