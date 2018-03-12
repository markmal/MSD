/*
 *	MSCDeviceClass.cpp
 *  Created on: Jan 27, 2018
 *      Author: mark
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


#include <stdint.h>
#include <Arduino.h>
#include <MSCDeviceClass.h>
#include <SCSIDeviceClass.h>
#include "SCSI.h"
#include <USB/USBAPI.h>
#include <USB/PluggableUSB.h>
#include "my_debug.h"
#include "LcdConsole.h"

#ifdef CDC_ENABLED
//#define MSC_DEVICE_CLASS_DEBUG
#endif

#ifdef MSC_DEVICE_CLASS_DEBUG
	#define print(str) Serial.print(str)
	#define println(str) Serial.println(str)
#else
	#define print(str)
	#define println(str)
#endif

void blink(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}


MSCDeviceClass::MSCDeviceClass() : PluggableUSBModule(NUM_ENDPOINTS, NUM_INTERFACE, epType)
{
#ifdef NVM_DEBUG
    pinMode(NVM_ENABLE_PIN, INPUT_PULLUP);
#endif

	debugPrint("Create MSC\n");
	bulkOutEndpoint = 0;
	bulkInEndpoint = 0;
	epType[0] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0); //tx
	epType[1] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0); //rx
	data = NULL;
	isHardStall = false;
	isInEndpointHalt = false;
	isOutEndpointHalt = false;
	align1=0;
	PluggableUSB().plug(this);
}

MSCDeviceClass::~MSCDeviceClass(void){}

int MSCDeviceClass::begin(){
	return scsiDev.begin();
}

String MSCDeviceClass::getSDCardError(){
	return scsiDev.getSDCardError();
}

String MSCDeviceClass::getError(){
	return "SD:"+scsiDev.getSDCardError() + "\nSCSI:"+scsiDev.getSCSIError();
}

int MSCDeviceClass::getInterface(uint8_t* interfaceCount)
{
	//if(!lcdConsole.isStarted()) lcdConsole.begin();
	//debugPrint("getInterface("+String(*interfaceCount)+")\n");

	*interfaceCount += 1; // uses 1

	MSCDescriptor MSCInterface = {
		D_INTERFACE(pluggedInterface, NUM_ENDPOINTS,
				USB_DEVICE_CLASS_STORAGE, MSC_SUBCLASS_SCSI,
				MSC_PROTOCOL_BULK_ONLY),
		D_ENDPOINT(USB_ENDPOINT_OUT(MSC_ENDPOINT_OUT),
				USB_ENDPOINT_TYPE_BULK, MSC_BLOCK_DATA_SZ, 0),
		D_ENDPOINT(USB_ENDPOINT_IN (MSC_ENDPOINT_IN),
				USB_ENDPOINT_TYPE_BULK, MSC_BLOCK_DATA_SZ, 0)
	};

	bulkOutEndpoint = MSC_ENDPOINT_OUT;
	bulkInEndpoint = MSC_ENDPOINT_IN;
	return USB_SendControl(0, &MSCInterface, sizeof(MSCInterface));
}

int MSCDeviceClass::getDescriptor(USBSetup& setup)
{
	//if(!lcdConsole.isStarted()) lcdConsole.begin();
	//debugPrint("getDescriptor\n");
	return 0;

	debugPrint("MSC::getDescriptor\n");
	debugPrint(" setup.bRequest:"+String(setup.bRequest,16)+"\n");
	debugPrint(" setup.bmRequestType:"+String(setup.bmRequestType,16)+"\n");
	return 0;

	if (setup.bmRequestType == REQUEST_DEVICETOHOST) {
		debugPrint("   setup.bmRequestType == REQUEST_DEVICETOHOST\n");
		debugPrint("   setup.wIndex: "+String(setup.wIndex)+"\n");
		debugPrint("   setup.wLength: "+String(setup.wLength)+"\n");
		debugPrint("   setup.wValueL: "+String(setup.wValueL)+"\n");
		debugPrint("   setup.wValueH: "+String(setup.wValueH)+"\n");
		return 0;
	}

	// In a MSC Class Descriptor wIndex contains the interface number
	if (setup.wIndex != pluggedInterface) {
		debugPrint("   setup.wIndex != pluggedInterface\n");
		return 0;
	}

	// Check if this is a MSC Class Descriptor request
	if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) {
		debugPrint("   setup.bmRequestType == REQUEST_DEVICETOHOST_STANDARD_INTERFACE\n");
		return 0;
	}
	//if (setup.wValueH != MSC_REPORT_DESCRIPTOR_TYPE) { return 0; }


	int total = 0;
	/*
	MSCSubDescriptor* node;
	for (node = rootNode; node; node = node->next) {
		int res = USB_SendControl(TRANSFER_PGM, node->data, node->length);
		if (res == -1)
			return -1;
		total += res;
	}
	*/
	// Reset the protocol on reenumeration. Normally the host should not assume the state of the protocol
	// due to the USB specs, but Windows and Linux just assumes its in report mode.
	//protocol = MSC_PROTOCOL_BULK_ONLY;
	
	return total;
}

uint8_t MSCDeviceClass::getShortName(char *name)
{
	debugPrint("MSCDeviceClass::getShortName\n");
	// this will be attached to Serial #. Use only unicode hex chars 0123456789ABCDEF
	memcpy(name, "0123456789ABCDEF", 17);
	return 0;
}

/*
void MSC_::AppendDescriptor(MSCSubDescriptor *node)
{
	if (!rootNode) {
		rootNode = node;
	} else {
		MSCSubDescriptor *current = rootNode;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}
	descriptorSize += node->length;
}
*/

/*
int MSC_::SendReport(uint8_t id, const void* data, int len)
{
	auto ret = USB_Send(pluggedEndpoint, &id, 1);
	if (ret < 0) return ret;
	auto ret2 = USB_Send(pluggedEndpoint | TRANSFER_RELEASE, data, len);
	if (ret2 < 0) return ret2;
	return ret + ret2;
}
*/


#ifdef NVM_DEBUG
#include <FlashStorage.h>
#endif

bool MSCDeviceClass::setup(USBSetup& setup)
{
	bool r = doSetup(setup);

	#ifdef NVM_DEBUG
	if (digitalRead(NVM_ENABLE_PIN) == 0)
	{
		  int len = debugLength();
		  if (len>1022) {len = 1022;}
		  CharsPage FlashStore;
		  FlashStore.len = 2;
		  memcpy(FlashStore.str, debugGetC(), len);
		  FlashStore.str[len-1] = 0;
		  FlashStorage(my_flash_store, CharsPage);
		  my_flash_store.write(FlashStore);
	}
	#endif

	return r;
}

uint8_t maxlun = 0;
bool MSCDeviceClass::doSetup(USBSetup& setup)
{
	if(!lcdConsole.isStarted()) lcdConsole.begin();
	debugPrint("MSC::setup\n");

	debugPrint("RqTp:"+String(setup.bmRequestType,16)
	+(" rcpt:"+String(setup.direction,16))
	+(" type:"+String(setup.type,16))
	+(" trDr:"+String(setup.transferDirection,16)));

	debugPrint("Rqst:"+String(setup.bRequest,16)
	+(" ValL:"+String(setup.wValueL,16))
	+(" ValH:"+String(setup.wValueH,16))
	+(" Indx:"+String(setup.wIndex,16))
	+(" Leng:"+String(setup.wLength,16)));

	uint8_t request = setup.bRequest;
	uint8_t requestType = setup.bmRequestType;
	uint8_t type = setup.type;
	uint8_t recipient = setup.direction;
	uint8_t transferDirection = setup.transferDirection;
	uint16_t length = setup.wLength;
	uint16_t value = setup.wValueL + (setup.wValueH << 8);
	uint16_t index = setup.wIndex;

	if (setup.type != USB_REQUEST_TYPE_CLASS) return false;

	switch (recipient) {
		case REQUEST_DEVICE:
			debugPrintln("RQ DVC:");
			return false;
		case REQUEST_INTERFACE:
			if (setup.wIndex != pluggedInterface){
				debugPrintln("Wrong INTFC:"+String(index)+" plgInt:"+String(pluggedInterface));
				return false;
			}
			switch (setup.bRequest) {
				case GET_STATUS: {
					debugPrintln("GET_STATUS");
					uint8_t buff[] = { 0, 0 };
					//USBDevice.armSend(0, buff, 2);
					uint32_t r = USBDevice.sendControl(buff, 2);
					debugPrintln("   r:"+String(r));
					return true;
					}
				case CLEAR_FEATURE: return true;
				case SET_FEATURE: return true;
				case GET_INTERFACE: return true; //  get the Alternative Interface
				case SET_INTERFACE: return true; //  set the Alternative Interface
				case MSC_GET_MAX_LUN: {
					debugPrint("MSC_GET_MAX_LUN\n");
					if (transferDirection!=1 || length!=1 || value!=0)
						return false; // USB2VC wants stall endpoint on incorrect params.
					if ((length!=1) || (value!=0)) return false; // stall
					uint32_t r = USBDevice.sendControl(&maxlun, 1);
					debugPrintln("   r:"+String(r));
					return true;
					}
				case MSC_RESET: {
					debugPrint("MSC_RESET ln:"+String(length));
					if (transferDirection!=0 && length!=0 && value!=0)
						return false;
					debugPrint("reset");
					if (reset()){
						USBDevice.sendZlp(0); // USB2VC wants this
						debugPrint("ZLP");
						return true;
					}
					debugPrint("NOZLP");
					return false; //false;
				}
			};// switch (setup.bRequest)
			break;
		case REQUEST_ENDPOINT: {
			debugPrint("REQUEST_ENDPOINT\n");
			USBSetupEndpoint sep = (USBSetupEndpoint&)index;
			uint8_t ep = sep.endpointNumber;
			uint8_t dir = sep.direction;
			debugPrintln("   ep:"+String(ep)+" dir:"+String(dir));
			if (!(ep == bulkInEndpoint || ep == bulkOutEndpoint)) return false;
			switch (setup.bRequest) {
				case GET_STATUS: {
					debugPrint("GET_STATUS\n");
					uint16_t isHalt = 0;
					if (ep == bulkInEndpoint) isHalt = (isInEndpointHalt)?1:0;
					else isHalt = (isOutEndpointHalt)?1:0;
					//USBDevice.armSend(0, &isHalt, 2);
					debugPrintln(" isHalt:"+String(isHalt));
					uint32_t r = USBDevice.sendControl(&isHalt, 2);
					debugPrintln("   r:"+String(r));
					return true;
				}
				case CLEAR_FEATURE:
					debugPrint("CLEAR_FEATURE\n");
					if (ep == bulkInEndpoint) isInEndpointHalt = false;
					else isOutEndpointHalt = false;
					USBDevice.sendZlp(0); // USB2VC wants this
					return true;
				case SET_FEATURE:
					debugPrint("SET_FEATURE\n");
					if (ep == bulkInEndpoint) isInEndpointHalt = true;
					else isOutEndpointHalt = true;
					USBDevice.sendZlp(0); // USB2VC wants this
					return true;
			};// switch (setup.bRequest)
			return false;
		};
		break;

		case REQUEST_OTHER:
			return false;
	}// switch (recipient)
	return false;

	/* -- all further is old variant
	if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE)
	{
		debugPrint("   requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE\n");
		debugPrint("   request:" + String(request) + " 0x"+String(request)+"\n");
		if (request == MSC_GET_MAX_LUN) {
			if ((length!=1) || (value!=0))
				return false; // USB2VC wants stall endpoint on incorrect params.
			debugPrint("     MSC_GET_MAX_LUN\n");
			if ((length!=1) || (value!=0)) return false; // stall

			int r = USBDevice.sendControl(pluggedEndpoint, &maxlun, 1);
			debugPrintln("   r:"+String(r));
			return true;
		}
		if (request == GET_STATUS)
			if( setup.bmRequestType == 2 ) { // Endpoint:
				if (isInEndpointHalt){
					uint8_t buff[] = { 1, 0 };
					USBDevice.sendControl(pluggedEndpoint, buff, 2 );
					return true;
				}
			}
	}

	if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE)
	{
		debugPrint("   requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE\n");
		debugPrint("   request:" + String(request) + " 0x"+String(request)+"\n");
		if (request == MSC_RESET) {
			debugPrint("     MSC_RESET\n");
			if ((length!=0) || (value!=0)) return false; // stall
			if (reset()){
				USBDevice.sendZlp(0); // USB2VC wants this
				return true;
			}
			else return false;
		}
		if (request == GET_STATUS)
			if( setup.bmRequestType == 2 ) { // Endpoint:
				if (isOutEndpointHalt){
					uint8_t buff[] = { 1, 0 };
					USBDevice.sendControl(pluggedEndpoint, buff, 2 );
					return true;
				}
			}
	}

	//digitalWrite(LED_BUILTIN, LOW);
	return false;
	*/
}

bool MSCDeviceClass::reset(){
	//println("reset");
	// TODO actual reset
	//scsiDev.lunReset();
	isOutEndpointHalt = false;
	isInEndpointHalt = false;
	isHardStall = false;
	return true;
}

void debugCBW(USB_MSC_CBW &cbw){
	debugPrint("      cbw.dCBWSignature:"+String(cbw.dCBWSignature)+"\n");
	debugPrint("      cbw.dCBWTag:"+String(cbw.dCBWTag)+"\n");
	debugPrint("      cbw.dCBWDataTransferLength:"+String(cbw.dCBWDataTransferLength)+"\n");
	debugPrint("      cbw.bmCBWFlags:"+String(cbw.bmCBWFlags)
			+ ((cbw.bmCBWFlags==USB_CBW_DIRECTION_IN)?"(IN)":
					((cbw.bmCBWFlags==0)?"(OUT)":"(BAD)"))
			+"\n");
	debugPrint("      cbw.bCBWLUN:"+String(cbw.bCBWLUN)+"\n");
	debugPrint("      cbw.bCBWCBLength:"+String(cbw.bCBWCBLength)+"\n");
	debugPrint("      cbw.CBWCB:");
	for (int i=0; i<cbw.bCBWCBLength; i++){
		debugPrint(String(cbw.CBWCB[i])+",");
	}
	debugPrint("\n");
}

/*
void debugPrintRespose(const SCSI_STANDARD_INQUIRY_DATA *inc){
	debugPrintlnSC("    inc.t10_vendor_id:", (char*)inc->inquiry.t10_vendor_id, 8);
	debugPrintlnSC("    inquiryData.inquiry.product_id:", (char*)inc->inquiry.product_id, 16);
}
*/

String MSCDeviceClass::getSCSIRequestInfo(){
	return scsiDev.requestInfo;
}

/*
 * Data-In - Indicates a transfer of data IN from the device to the host.
 */
uint32_t MSCDeviceClass::receiveInRequest(){
	//lcdConsole.println("IN:"+ String(cbw.dCBWDataTransferLength));

	debugPrint("USB_CBW_DIRECTION_IN: len:" + String(cbw.dCBWDataTransferLength)+"\n");
	//uint16_t rlen = cbw.dCBWDataTransferLength;
	uint32_t tfLen = cbw.dCBWDataTransferLength;

	//uint8_t* response = NULL;
	//if (rlen) response = (uint8_t*)malloc(rlen);
	//debugPrintlnSX("  cbw.CBWCB:",cbw.CBWCB, cbw.bCBWCBLength);

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);
	//debug+=" cbd.read10.LBA:"+String(cbd.read10.LBA)+" cbd.read10.length:"+String(cbd.read10.length)+"\n");

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength;

	int txlen = scsiDev.handleRequest(cbd, tfLen);

	//lcdConsole.println("  txlen:"+ String(txlen));
	//SerialUSB.println("  txlen:"+ String(txlen));
	// negative txlen means ERROR
	int slen=0, sl=0; int rlen=0; int rl=txlen;

	if (txlen > 0) {
		while (slen < txlen && rl>0){
			debugPrint("about scsiDev.readData...");
			rl = scsiDev.readData(data);
			debugPrint("scsiDev.readData read:"+String(rl)+"\n");
			//lcdConsole.println("  read rl:"+ String(rl));
			//SerialUSB.println("  read rl:"+ String(rl));
			if (rl < 0) return -1; // error
			rlen += rl;
			//SerialUSB.println("  send rl:"+ String(rl));
			debugPrint("  send rl:"+ String(rl)+"\n");
			sl = USBDevice.send(bulkInEndpoint, data, rl);
			debugPrint("  sent sl:"+ String(sl)+"\n");
			//SerialUSB.println("  sent sl:"+ String(sl));
			slen += sl;
			//SerialUSB.println("  slen:"+ String(slen));
			//lcdConsole.println("  txlen:"+ String(txlen)+" slen:"+ String(slen));
		}

		/* For Data-In the device shall report in the dCSWDataResidue
		 * the difference between the amount of data expected as stated
		 * in the dCBWDataTransferLength and the actual amount of relevant
		 * data sent by the device. */
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - slen;
	}

	if (scsiDev.scsiStatus == GOOD) {
		csw.bCSWStatus = USB_CSW_STATUS_PASS;
	}else{
		csw.bCSWStatus = USB_CSW_STATUS_FAIL;
	}

	USBDevice.send(bulkInEndpoint, &csw, USB_CSW_SIZE);
	//USBDevice.flush(txEndpoint);

	/* try ????
	if (scsiDev.scsiStatus != GOOD)
		USBDevice.stall(txEndpoint);
	*/
	return slen;
}

/*
 * Data-Out Indicates a transfer of data OUT from the host to the device.
 */
uint32_t MSCDeviceClass::receiveOutRequest(){ // receives block from USB
	//lcdConsole.println("OUT:"+ String(cbw.dCBWDataTransferLength));
	//println("OUT:"+ String(cbw.dCBWDataTransferLength));
	debugPrint("USB_CBW_DIRECTION_OUT: len:" + String(cbw.dCBWDataTransferLength)+"\n");
	//uint16_t rlen = cbw.dCBWDataTransferLength;
	uint32_t tfLen = cbw.dCBWDataTransferLength;

	//uint8_t* response = NULL;
	//if (rlen) response = (uint8_t*)malloc(rlen);
	//debugPrintlnSX("  cbw.CBWCB:",cbw.CBWCB, cbw.bCBWCBLength);

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);
	//debug+=" cbd.read10.LBA:"+String(cbd.read10.LBA)+" cbd.read10.length:"+String(cbd.read10.length)+"\n");

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;

	int txlen = scsiDev.handleRequest(cbd, tfLen);
	//lcdConsole.println("  txlen:"+ String(txlen));
	//println("  txlen:"+ String(txlen));

	uint32_t wlen=0, rl=0; int rlen=0; int wl=txlen;
	uint32_t rcvl=0;

	if (txlen > 0) {
		while (wlen < txlen && wl>0){
			rl=0;
			rcvl = scsiDev.getMaxTransferLength();
			if (txlen<rcvl) rcvl=txlen;
			if ((txlen-wlen) < rcvl) rcvl = txlen-wlen;
			while (rl < rcvl ){
				uint32_t ms1 = millis(); uint32_t waittime=0;
				while (USBDevice.available(bulkOutEndpoint) < ((256 < (rcvl-rl))?256:(rcvl-rl))
						&& (waittime < USB_READ_TIMEOUT_MS)) {
					//delay(1);
					waittime = millis() - ms1;
				}
				if (waittime >= USB_READ_TIMEOUT_MS) {
					csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
					println("USB_RECEIVE_TIMEOUT");
					goto USB_RECEIVE_ERROR;
				}
				println("      rl:"+ String(rl));

				int r = USBDevice.recv(bulkOutEndpoint, data+rl, rcvl-rl);

				println("  recv r:"+ String(r));
				rl+=r;
				println("      next rl:"+ String(rl) + " of rcvl:"+ String(rcvl));
			}
			rlen += rl;
			println("  rlen:"+ String(rlen));
			wl = scsiDev.writeData(data);
			wlen += wl;
			println("  wlen:"+ String(wlen) + " of txlen:"+ String(txlen));

			if (scsiDev.scsiStatus != GOOD) {
				csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
				println("USB_CSW_STATUS_PHASE_ERROR");
				goto SCSI_WRITE_ERROR;
			}

		}
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - wlen;
	}

	if (scsiDev.scsiStatus == GOOD) {
		csw.bCSWStatus = USB_CSW_STATUS_PASS;
	}else{
		csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
		goto SCSI_WRITE_ERROR;
	}

	USB_RECEIVE_ERROR:
	SCSI_WRITE_ERROR:

	//debug+="  USB_Send CSW\n");
	//print(debug); debug="";
	USBDevice.send(bulkInEndpoint, &csw, USB_CSW_SIZE);
	//debug+="  USB_Sent CSW\n");
	//print(debug); debug="";
	USBDevice.flush(bulkInEndpoint);

	return wlen;
}

bool MSCDeviceClass::checkCBW(USB_MSC_CBW& cbw) {
	if (cbw.dCBWSignature != USB_CBW_SIGNATURE) return false;
	if (cbw.bCBWLUN != 0) return false;
	if (cbw.bCBWCBLength < 1 || cbw.bCBWCBLength > 16 ) return false;
	if ( (cbw.bmCBWFlags & 0xE) != 0) return false;
}

int err=0;

uint32_t MSCDeviceClass::receiveRequest(){ // receives block from USB
	//debugPrint("receiveRequest()");
	//print(debug); debug="";
	uint32_t rxa = USBDevice.available(bulkOutEndpoint);
	//USBDevice.recv(bulkOutEndpoint, &cbw, 0);
	//debugPrint("  USBDevice.available rx:"+String(rxa)+"\n");
	//print(debug); debug="";

	if (rxa >= USB_CBW_SIZE) {
		//println("avail, receiving "+String(rxa));
		int r = USBDevice.recv(bulkOutEndpoint, &cbw, USB_CBW_SIZE);
		debugPrintln("rcvd:"+String(r));
		if (r==31 && checkCBW(cbw)){
			debugCBW(cbw);
			if(isHardStall) {
				debugPrintln("in hard stall");
				USBDevice.stall(0);
				return FAILURE;
			}
			//isInEndpointHalt = false;
			//isOutEndpointHalt = false;
			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_IN){ // from the device to the host.
				if(isInEndpointHalt) {USBDevice.stall(0); return FAILURE;}
				receiveInRequest();
			} else
			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_OUT){ // from the host to the device.
				if(isOutEndpointHalt) {USBDevice.stall(0); return FAILURE;}
				receiveOutRequest();
			}
		}else {
			err++;
			debugPrintln("Bad CBW");

			isHardStall = true;

			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_IN){
				isInEndpointHalt = true;
				debugPrintln("stall bulkInEndpoint");
				USBDevice.stall(bulkInEndpoint);
			}
			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_OUT){
				isOutEndpointHalt = true;
				debugPrintln("stall bulkOutEndpoint");
				USBDevice.stall(bulkOutEndpoint);
			}
			debugPrintln("stall Ctrl");
			USBDevice.stall(0);
			return FAILURE;
		}
		return r;
	}
	else {
		if (rxa>0) debugPrint("avlb rx:"+String(rxa));
		scsiDev.requestInfo="";
		return 0;
	}
}

/*
uint32_t MSC_::sendBlock(){ // sends block to USB
	debugPrint(" MSC_::sendBlock()\n");
	return USB_Send(txEndpoint, blockData, BLOCK_DATA_SZ);
}
*/
//#endif /* if defined(USBCON) */
