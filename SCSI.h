/*
 * SCSI.h
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


#define SCSI_INQUIRY_SUPPORTED_VPD_PAGES_PAGES	0x00
#define SCSI_INQUIRY_UNIT_SERIAL_NUMBER_PAGE	0x80
#define SCSI_INQUIRY_DEVICE_IDENTIFICATION_PAGE	0x83

_Pragma("pack(1)")

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

struct SCSI_INQUIRY_UNIT_SERIAL_NUMBER_PAGE_DATA {
	uint8_t peripheral_device_type:5, // 0 - LUN 0
			peripheral_qualifier:3;   // 0x00 - Direct Access Device
	uint8_t page_code; // 0x80
	uint8_t reserv1;
	uint8_t page_length; // 16
	uint8_t product_serial_number[16];
};
/* -- from Kingston usb flash
       <header      > < descriptors ...
0000   00 80 02 02 1f 00 00 00 4b 69 6e 67 73 74 6f 6e  ........Kingston
0010   44 61 74 61 54 72 61 76 65 6c 65 72 20 32 2e 30  DataTraveler 2.0
0020   31 2e 30 30 30 00 30 00 30 00 34 00 36 00 41 00  1.000.0.0.4.6.A.
0030   42 00 30 00 30 00 30 00 30 00 30 00 30 00 30 00  B.0.0.0.0.0.0.0.
0040   30 00 30 00 30 00 30 00 30 00 34 00 36 00 41 00  0.0.0.0.0.4.6.A.
0050   42 00 ff f0 a3 74 47 f0 90 90 08 74 57 f0 a3 74  B....tG....tW..t
0060   9c f0 a3 74 ff f0 a3 74 6d f0 80 0f 74 62 f0 a3  ...t...tm...tb..
0070   74 ba f0 a3 74 ff f0 a3 74 d3 f0 90 00 ef 02 53  t...t...t......S
0080   f6 12 5f 5d 12 65 08 90 20 00 e0 f5 30 a3 e0 f5  .._].e.. ...0...
0090   31 75 33 00 75 34 01 d3 e5 34 95 31 e5 33 95 30  1u3.u4...4.1.3.0
00a0   50 2d 12 96 cc 33 fe e4 2f f5 82 74 20 3e f5 83  P-...3../..t >..
00b0   a3 e0 25 e0 ff 05 82 d5 82 02 15 83 15 82 e0 12  ..%.............
00c0   45 34 12 1f c0 05 34 e5 34 70 02 05 33 80 c8 02  E4....4.4p..3...
00d0   91 00 02 41 69 7e 00 7f 40 7d 00 90 01 55 12 05  ...Ai~..@}...U..
00e0   9a 12 08 32 90 60 12 e0 20 e0 0e 90 01 55 12 05  ...2.`.. ....U..
00f0   9a 74 80 90 00 03 12 01 97 90 01 58 e0 24 fb     .t.........X.$.
*/
struct IDENTIFICATION_DESCRIPTOR_DATA {
	uint8_t code_set:4, protocol_identifier:4;
	uint8_t identifier_type:4, association:2, reserv0:1, PIV:1;
	uint8_t reserv1;
	uint8_t identifier_length; // n-3
	uint8_t identifier[8];
};

struct SCSI_INQUIRY_DEVICE_IDENTIFICATION_DATA {
	uint8_t peripheral_device_type:5, // 0 - LUN 0
			peripheral_qualifier:3;   // 0x00 - Direct Access Device
	uint8_t page_code; // 0x83
	uint8_t reserv1;
	uint16_t page_length; // MSB,LSB
	IDENTIFICATION_DESCRIPTOR_DATA identification_descriptor_list[1];
};

struct SCSI_INQUIRY_SUPPORTED_VPD_PAGES_DATA {
	uint8_t peripheral_device_type:5, // 0 - LUN 0
			peripheral_qualifier:3;   // 0x00 - Direct Access Device
	uint8_t page_code; // 0x00
	uint8_t reserv1;
	uint8_t page_length; // 3
	uint8_t supported_page_list[2]; // 0x00, 0x80, 0x83
};


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

#define FORMAT_CAPACITY_DESCRIPTOR_CODE_UNFORMATTED_MEDIA_MAX	0x01
#define FORMAT_CAPACITY_DESCRIPTOR_CODE_FORMATTED_MEDIA_CUR 	0x02
#define FORMAT_CAPACITY_DESCRIPTOR_CODE_NO_CARTRIGE_MAX			0x03

struct FORMAT_CAPACITY_DESCRIPTOR {
	uint32_t numer_of_blocks;
	uint8_t descritpor_code:2, reserv1:6;
	uint8_t block_length[3];
};

struct SCSI_CBD_READ_FORMAT_CAPACITIES_DATA {
	struct CAPACITY_LIST_HEADER {
		uint8_t reserv1;
		uint8_t reserv2;
		uint8_t reserv3;
		uint8_t capacity_list_length; // Capacity List Length field specifies the length in bytes of the Capacity Descriptors that follow. Each
									  // Capacity Descriptor is eight bytes in length
	} capacity_list_header;

	FORMAT_CAPACITY_DESCRIPTOR maximum_capacity_descritpor;
	FORMAT_CAPACITY_DESCRIPTOR formattable_capacity_descritpors[1]; // need one for now
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

_Pragma("pack()")

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
