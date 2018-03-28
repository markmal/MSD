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


MSCDeviceClass::MSCDeviceClass() : PluggableUSBModule(
		NUM_ENDPOINTS, NUM_INTERFACE, epType)
{
#ifdef NVM_DEBUG
    pinMode(NVM_ENABLE_PIN, INPUT_PULLUP);
#endif

	debugPrint("Create MSC\n");
	bulkOutEndpoint = 0;
	bulkInEndpoint = 0;
	bulkOutEndpointAddr = 0;
	bulkInEndpointAddr = 0;
	epType[0] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0); //tx
	epType[1] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0); //rx
	data = NULL;
	isHardStall = false;
	isInEndpointHalt = false;
	isOutEndpointHalt = false;

	isBOTMSCReset = false;
	isInEndpointStall = false;
	isOutEndpointStall = false;

	align1=0;
	bool r = PluggableUSB().plug(this);
	if (r) debugPrint("plugged");
	else debugPrint("NOT plugged");
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
	bulkOutEndpointAddr = USB_ENDPOINT_OUT(MSC_ENDPOINT_OUT);
	bulkInEndpointAddr = USB_ENDPOINT_IN(MSC_ENDPOINT_IN);

	epType[0] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(MSC_ENDPOINT_OUT); //tx
	epType[1] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(MSC_ENDPOINT_IN); //rx

	//debugPrint("getInterface("+String(*interfaceCount)+")\n");

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
	//debugPrintln("inSetup");
	bool r = doSetup(setup);
	//if (r) debugPrintln(" OK"); else debugPrintln(" ST");
	return r;
}



bool MSCDeviceClass::doSetup(USBSetup& setup)
{

	//debugPrint("MSC::setup\n");

	/*
	debugPrint(//"RT:"+String(setup.bmRequestType,16)
	("RC:"+String(setup.direction,16)) // receiver
	+(" TP:"+String(setup.type,16))
	+(" TD:"+String(setup.transferDirection,16)));

	debugPrint(" RQ:"+String(setup.bRequest,16)
	+(" V:"+String(256*setup.wValueL+setup.wValueH,16))
	+(" IX:"+String(setup.wIndex,16))
	+(" LN:"+String(setup.wLength,16)));
	*/
	uint8_t recipient = setup.direction;

	/* Standard requests to custom Intfs and EPs also sent here
	if (setup.type != USB_REQUEST_TYPE_CLASS) {
		USBDevice.sendZlp(0); // USB2VC wants this
		return false;
	}
	*/

	switch (recipient) {
		case REQUEST_DEVICE:
			//debugPrintln("RQ DVC:");
			switch (setup.bRequest) {
				case SET_CONFIGURATION:
					//debugPrintln("DEV SETCFG");
					USBDevice.initEP(0,USB_ENDPOINT_TYPE_CONTROL);
					USBDevice.flush(bulkInEndpoint);
					USBDevice.flushOut(bulkOutEndpoint);
					USBDevice.initEndpoints();
					USBDevice.clearToggleIn(bulkInEndpoint);
					USBDevice.clearToggleOut(bulkInEndpoint);
					USBDevice.clearToggleIn(bulkOutEndpoint);
					USBDevice.clearToggleOut(bulkOutEndpoint);
					//USBDevice.sendZlp(bulkInEndpoint);
					return true;
			}
			return false;
		case REQUEST_INTERFACE:
			if (setup.wIndex != pluggedInterface){
				//debugPrintln("Wrong INTFC:"+String(index)+" plgInt:"+String(pluggedInterface));
				return false;
			}
			switch (setup.bRequest) {
				case GET_STATUS: {
					//debugPrintln("GET_STATUS");
					uint8_t buff[] = { 0, 0 };
					//USBDevice.armSend(0, buff, 2);
					uint32_t r = USBDevice.sendControl(buff, 2);
					//debugPrintln("   r:"+String(r));
					return true;
					}
				case CLEAR_FEATURE: return true;
				case SET_FEATURE: return true;
				case GET_INTERFACE: return true; //  get the Alternative Interface
				case SET_INTERFACE: return true; //  set the Alternative Interface
				case MSC_GET_MAX_LUN: {
					//debugPrint("MSC_GET_MAX_LUN\n");
					if (setup.transferDirection!=1 || setup.wLength!=1
						       || setup.wValueL!=0 || setup.wValueH!=0){
						//debugPrint("STALL");
						return false; // USB2VC wants stall endpoint on incorrect params.
					}
					uint8_t maxlun = 0;
					uint32_t r = USBDevice.sendControl(&maxlun, 1);
					//debugPrintln("   r:"+String(r));
					return true;
					}
				case MSC_RESET: {
					//USBDevice.debugPrint("MSC_RESET");
					if (setup.transferDirection!=0 || setup.wLength!=0
						       || setup.wValueL!=0 || setup.wValueH!=0){
						//debugPrint("STALL");
						return false; // USB2VC wants stall endpoint on incorrect params.
					}
					//debugPrint("reset");
					if (reset()){
						//debugPrint("ZLP");
						return true;
					}
					//debugPrint("NOZLP");
					return false; //false;
				}
			};// switch (setup.bRequest)
			break;
		case REQUEST_ENDPOINT: {
			//USBDevice.debugPrint("RQEP:"+String(setup.wIndex,16));
			if ( setup.wIndex != bulkInEndpointAddr && setup.wIndex != bulkOutEndpointAddr)
				return false;
			switch (setup.bRequest) {
				case GET_STATUS: {
					//USBDevice.debugPrint(" GTST");
					uint8_t isHalt[] = { 0, 0 };
					if( isHardStall
					  ||(setup.wIndex == bulkInEndpointAddr
							&& (isInEndpointHalt || isInEndpointStall))
					  ||(setup.wIndex == bulkOutEndpointAddr
							&& (isOutEndpointHalt || isOutEndpointStall))
					)
						isHalt[0] = 1;
					//USBDevice.debugPrint(" isHalt:"+String(isHalt[0])+"\n");
					//USBDevice.armSend(0, isHalt, 2);
					USBDevice.sendControl(isHalt, 2);
					return true;
				}
				case CLEAR_FEATURE:
					//USBDevice.debugPrint(" CLRFTR:");
					if (setup.wIndex == bulkInEndpointAddr) {
						// MSC Error Recovery test
						if (isBOTMSCReset && (isInEndpointStall || isHardStall)) {
							USBDevice.debugPrint("InitIn");
							uint8_t epType =  USB_ENDPOINT_TYPE_BULK | bulkInEndpointAddr;
							USBDevice.initEP(bulkInEndpoint, epType);
							//if (USBDevice.isStallIn(bulkInEndpoint))
							USBDevice.flush(bulkInEndpoint);
							USBDevice.clearStallIn(bulkInEndpoint);
							//if (USBDevice.isStallOut(bulkInEndpoint))
							USBDevice.clearStallOut(bulkInEndpoint);
							//USBDevice.clearToggleIn(bulkInEndpoint);
							//USBDevice.clearToggleOut(bulkInEndpoint);
							isInEndpointStall = false;
						}
						USBDevice.debugPrint("\n");

						if (isInEndpointHalt) // Chapter 9 Endpoint Halt Test
						  isInEndpointHalt = false;

						USBDevice.sendZlp(bulkInEndpoint); // USB2VC wants this
					}
					if (setup.wIndex == bulkOutEndpointAddr) {

						// MSC Error Recovery test
						if (isBOTMSCReset && (isOutEndpointStall || isHardStall)) {
							//USBDevice.debugPrint("InitOut");
							USBDevice.clearStallOut(bulkOutEndpoint); // this clears stall
							USBDevice.initEP(bulkOutEndpoint, (USB_ENDPOINT_TYPE_BULK | bulkOutEndpointAddr));
							//if (USBDevice.isStallOut(bulkOutEndpoint))
							//USBDevice.clearStallOut(bulkOutEndpoint);
							//USBDevice.flushOut(bulkOutEndpoint);
							//USBDevice.clearStallIn(bulkOutEndpoint);
							//USBDevice.clearToggleIn(bulkOutEndpoint);
							//USBDevice.clearToggleOut(bulkOutEndpoint);
							//USBDevice.debugPrint("OutPipAvl:"+String( USBDevice.available(bulkOutEndpoint))+"\n");
							isOutEndpointStall = false;
						}
						//USBDevice.debugPrint("\n");

						if (isOutEndpointHalt) // Chapter 9 Endpoint Halt Test
							isOutEndpointHalt = false;
					}
					/* isBOTMSCReset set by MSC_RESET.
					 * isBOTMSCReset is cleared when it is:
					 * isBOTMSCReset=true and isInEndpointHalt=false and isOutEndpointHalt=false
					 */
					if (isBOTMSCReset && (!isInEndpointStall && !isOutEndpointStall)){
						isBOTMSCReset = false;
						isHardStall = false;
					}

					USBDevice.sendZlp(0); // USB2VC wants this
					return true;
				case SET_FEATURE:
					//USBDevice.debugPrint(" SETFTR\n");
					if (setup.wIndex == bulkInEndpointAddr) {
						isInEndpointHalt = true;
						//USBDevice.sendZlp(bulkInEndpoint); // USB2VC wants this
						USBDevice.stallIn(bulkInEndpoint);
					}
					if (setup.wIndex == bulkOutEndpointAddr) {
						isOutEndpointHalt = true;
						USBDevice.stallOut(bulkOutEndpoint);
					}
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
}


bool MSCDeviceClass::reset(){
	//println("reset");
	// TODO actual reset
	//scsiDev.lunReset();
	//USBDevice.sendZlp(0); // USB2VC wants this
	//USBDevice.sendZlp(bulkInEndpoint); // USB2VC wants this
	/*uint32_t a = USBDevice.available(bulkOutEndpoint);
	if (a){
		USBDevice.debugPrint(" a:"+String(a)+"\n");
		uint8_t buff[a];
		USBDevice.recv(bulkOutEndpoint, buff, a);
	}
	*/
	//isOutEndpointHalt = false;
	//isInEndpointHalt = false;
	//isHardStall = false;
	isBOTMSCReset = true;
	USBDevice.debugPrint(" RESET\n");
	return true;
}

void debugCBW(USB_MSC_CBW &cbw){
	debugPrint("CBWSig:"+String(cbw.dCBWSignature,16)+"\n");
	debugPrint("CBWTag:"+String(cbw.dCBWTag,16)+"\n");
	/*debugPrint("CBWDTL:"+String(cbw.dCBWDataTransferLength));
	if (cbw.bmCBWFlags.direction==USB_CBW_DIRECTION_IN) debugPrintln("(IN)");
	else debugPrintln("(OUT)");
	debugPrint("CBWLUN:"+String(cbw.bCBWLUN)+"\n");
	debugPrint("CBWCBLen:"+String(cbw.bCBWCBLength)+"\n");
	*/
	debugPrint("cbw.CBWCB:");
	for (int i=0; i<cbw.bCBWCBLength; i++){
		debugPrint(String(cbw.CBWCB[i],16)+",");
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
	debugPrint("RQIN:dtl:" + String(cbw.dCBWDataTransferLength)+"\n");
	uint32_t tfLen = cbw.dCBWDataTransferLength;
	int iDTL = cbw.dCBWDataTransferLength; // int DTL needs for proper comparisons below

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength;

	int txl = scsiDev.handleRequest(cbd, tfLen);
	uint32_t txlen = 0;
	if (txl >= 0) txlen = txl;

	//uint8_t rCase = scsiDev.getMSCResultCase();

	// negative txlen means ERROR
	uint32_t slen=0, sl=0;
	//uint32_t rlen=0;
	int rl=txl;

	if (txlen > 0) {
		while ((slen < txlen) && (slen < cbw.dCBWDataTransferLength) && (rl>0)){
			//debugPrint("about scsiDev.readData...");
			rl = scsiDev.readData(data);
			//debugPrintln("rl:"+String(rl));
			if (rl < 0){ // SCSI error
				break;
			}
			uint32_t rlu = rl;
			if(rlu>(cbw.dCBWDataTransferLength-slen))
				rlu=cbw.dCBWDataTransferLength-slen;
			//debugPrintln("rl:"+ String(rlu));
			sl = USBDevice.send(bulkInEndpoint, data, rlu);
			//debugPrintln("sl:"+ String(sl));
			//SerialUSB.println("  sent sl:"+ String(sl));
			slen += sl;
			//SerialUSB.println("  slen:"+ String(slen));
			//lcdConsole.println("  txlen:"+ String(txlen)+" slen:"+ String(slen));
		}
	}
	//debugPrintln("txlen:"+ String(txlen)+" slen:"+ String(slen));

	/* For Data-In the device shall report in the dCSWDataResidue
	 * the difference between the amount of data expected as stated
	 * in the dCBWDataTransferLength and the actual amount of relevant
	 * data sent by the device. */
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength - slen;

	if ((scsiDev.scsiStatus == GOOD) && (slen == txlen)) {
		csw.bCSWStatus = USB_CSW_STATUS_PASS;
	}else{
		csw.bCSWStatus = USB_CSW_STATUS_FAIL;
	}

	if (cbw.dCBWDataTransferLength == 0){
		if (txlen > 0)	//case 2
			csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
	} else
	if (cbw.dCBWDataTransferLength > 0){
		if (txlen < cbw.dCBWDataTransferLength
				|| slen < cbw.dCBWDataTransferLength
				|| txl < 0 )	{ //case 4,5
			debugPrintln("case 4,5");
			uint32_t bDTL = cbw.dCBWDataTransferLength / 512 * 512;
			uint32_t sl = slen;
			while(sl < bDTL){
				sl += USBDevice.send(bulkInEndpoint, data, 512); // send garbage till DTL reached
			}
			if (cbw.dCBWDataTransferLength > sl)
				USBDevice.send(bulkInEndpoint, data, cbw.dCBWDataTransferLength - sl); // send garbage till DTL reached
			csw.bCSWStatus = USB_CSW_STATUS_FAIL;
		}
		else if (txlen > cbw.dCBWDataTransferLength)	{ //case 7
			debugPrintln("case 7");
			drainInPipe(cbw.dCBWDataTransferLength - slen);
			/*
			if (slen < cbw.dCBWDataTransferLength){
				debugPrintln("slen:"+String(slen));
				USBDevice.send(bulkInEndpoint, data, cbw.dCBWDataTransferLength - slen);
			}else{
				USBDevice.sendZlp(bulkInEndpoint); // send short packet
			}*/
			csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
		}
	}

	USBDevice.send(bulkInEndpoint, &csw, USB_CSW_SIZE);
	//USBDevice.flush(bulkInEndpoint);

	return slen;
}

/*
 * Data-Out Indicates a transfer of data OUT from the host to the device.
 */
uint32_t MSCDeviceClass::receiveOutRequest(){ // receives block from USB
	debugPrintln("RQOUT:dtl:" + String(cbw.dCBWDataTransferLength));
	uint32_t tfLen = cbw.dCBWDataTransferLength;

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;

	int txl = scsiDev.handleRequest(cbd, tfLen);
	debugPrintln("txl:"+String(txl));
	uint32_t txlen = 0;
	if (txl >= 0) txlen = txl;

	uint32_t wlen=0, rl=0, rlen=0;
	int wl=txl;
	uint32_t rcvl=0;

	if (txlen > 0) {
		while ((wlen < txlen) && (rlen < cbw.dCBWDataTransferLength) && (wl>0)){
			rl=0;
			rcvl = scsiDev.getMaxTransferLength();
			if (txlen<rcvl) rcvl=txlen;
			if ((txlen-wlen) < rcvl) rcvl = txlen-wlen;
			while (rl < rcvl ){
				uint32_t ms1 = millis(); uint32_t waittime=0;
				while (USBDevice.available(bulkOutEndpoint) < ((256 < (rcvl-rl))?256:(rcvl-rl))
						&& (waittime < USB_READ_TIMEOUT_MS)) {
					waittime = millis() - ms1;
				}
				if (waittime >= USB_READ_TIMEOUT_MS) {
					//csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
					debugPrintln("USB_RECEIVE_TIMEOUT");
					rlen += rl;
					goto USB_RECEIVE_ERROR;
				}
				//debugPrintln("rl:"+ String(rl));

				uint32_t r = USBDevice.recv(bulkOutEndpoint, data+rl, rcvl-rl);

				//println("  recv r:"+ String(r));
				rl+=r;
				//println("      next rl:"+ String(rl) + " of rcvl:"+ String(rcvl));
			}
			rlen += rl;
			//println("  rlen:"+ String(rlen));
			wl = scsiDev.writeData(data);
			//debugPrintln("wl:"+String(rl));
			if (wl < 0){ // SCSI error
				break;
			}
			wlen += wl;

			if (scsiDev.scsiStatus != GOOD) {
				csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
				println("USB_CSW_STATUS_PHASE_ERROR");
				goto SCSI_WRITE_ERROR;
			}

		}
	}

	USB_RECEIVE_ERROR:
	SCSI_WRITE_ERROR:

	//debugPrintln("  rlen:"+ String(rlen) + " of txlen:"+ String(txlen));
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength - rlen;
	//debugPrintln("RZD:"+String(csw.dCSWDataResidue));

	if ((scsiDev.scsiStatus == GOOD) && (rlen == txlen) && (wlen == txlen)) {
		csw.bCSWStatus = USB_CSW_STATUS_PASS;
	}else{
		csw.bCSWStatus = USB_CSW_STATUS_FAIL;
	}

	if (cbw.dCBWDataTransferLength == 0){
		if (txlen > 0)	//case 3
			csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
	} else
	if (cbw.dCBWDataTransferLength > 0){
		if (txlen < cbw.dCBWDataTransferLength || rlen < txlen || txl<0)	{ //case 9,11
			if (txlen < cbw.dCBWDataTransferLength)	{ //case 13
				debugPrintln("case 9,11");
				csw.bCSWStatus = USB_CSW_STATUS_FAIL;
			} else
			if (txlen > cbw.dCBWDataTransferLength)	{ //case 13
				debugPrintln("case 13");
				csw.bCSWStatus = USB_CSW_STATUS_PHASE_ERROR;
			}

			drainOutPipe(cbw.dCBWDataTransferLength - rlen);
		}
	}

	//debug+="  USB_Send CSW\n");
	//print(debug); debug="";
	USBDevice.send(bulkInEndpoint, &csw, USB_CSW_SIZE);
	//debug+="  USB_Sent CSW\n");
	//print(debug); debug="";
	//USBDevice.flush(bulkInEndpoint);

	return wlen;
}

void MSCDeviceClass::drainInPipe(uint32_t len) {
	debugPrintln("drainInPipe:"+String(len));
	uint32_t bDTL = len / 512 * 512;
	uint32_t slen = 0;
	while(slen < bDTL){
		slen += USBDevice.send(bulkInEndpoint, data, 512);
		//debugPrintln("sent:"+String(slen));
	}
	while(slen < len){
		slen += USBDevice.send(bulkInEndpoint, data, len - slen);
		//debugPrintln("rest:"+String(slen));
	}
}

void MSCDeviceClass::drainOutPipe(uint32_t len) {
	uint32_t bDTL = len / 512 * 512;
	uint32_t rlen = 0;
	while(rlen < bDTL){
		rlen += USBDevice.recv(bulkOutEndpoint, data, 512);
	}
	while(rlen < len)
		rlen += USBDevice.recv(bulkOutEndpoint, data, len - rlen);
}

bool MSCDeviceClass::isCBWValid(USB_MSC_CBW& cbw) {
	if (cbw.dCBWSignature != USB_CBW_SIGNATURE) {
		debugCBW(cbw);
		debugPrintln("bad Sig");
		return false;
	}
	return true;
}
bool MSCDeviceClass::isCBWMeaningful(USB_MSC_CBW& cbw) {
	if (cbw.bCBWLUN != 0) {
		debugCBW(cbw);
		debugPrintln("bad LUN");
		return false;
	}
	if (cbw.bCBWCBLength < 1 || cbw.bCBWCBLength > 16 ) {
		debugCBW(cbw);
		debugPrintln("bad Len");
		return false;
	}
	if ( (cbw.bmCBWFlags.obsolete != 0) || (cbw.bmCBWFlags.reserved != 0) ) {
		debugCBW(cbw);
		debugPrintln("bad Flg");
		return false;
	}

	//SCSI_CBD* cbdp = (SCSI_CBD*)(cbw.CBWCB);
	SCSI_CBD cbd;
	memcpy(&cbd, cbw.CBWCB,16);
	if ( scsiDev.isRequestMeaningful(cbd,
			cbw.dCBWDataTransferLength,
			cbw.bmCBWFlags.direction) < 0) {
		debugCBW(cbw);
		debugPrintln("Rq!Mnfl");
		return false;
	}

	return true;
}

int err=0;

void MSCDeviceClass::hardStall(){
	USBDevice.stallIn(bulkInEndpoint);
	//USBDevice.stallOut(bulkInEndpoint);
	isInEndpointStall = true;
	USBDevice.stallOut(bulkOutEndpoint);
	//USBDevice.stallIn(bulkOutEndpoint);
	isOutEndpointStall = true;
	isHardStall = true;
}

uint32_t MSCDeviceClass::receiveRequest(){ // receives block from USB
	//debugPrint("receiveRequest()");
	if (! USBDevice.configured()) {
		//debugPrintln("UNCONFIGURED");
		return FAILURE;
	}

	uint32_t rxa = USBDevice.available(bulkOutEndpoint);
	if(rxa==0) return 0;

	debugPrintln("rxa:"+String(rxa));

	if(isHardStall){
		debugPrintln("INHRDSTL");
		hardStall();
		return FAILURE;
	}

	if (rxa < USB_CBW_SIZE) {
		return 0;
	}
	//println("avail, receiving "+String(rxa));
	uint32_t r = USBDevice.recv(bulkOutEndpoint, &cbw, USB_CBW_SIZE);
	//USBDevice.debugPrint("rcvd:"+String(r));
	if (r!=31) {
		hardStall();
		return FAILURE;
	}

	//debugCBW(cbw);

	if ( ! isCBWValid(cbw)){
		hardStall();
		return FAILURE;
	}

	if ( ! isCBWMeaningful(cbw)){ // Case 8
		if (cbw.bmCBWFlags.direction == USB_CBW_DIRECTION_IN){
			debugPrintln("drainInPipe");
			drainInPipe(cbw.dCBWDataTransferLength);
			USB_MSC_CSW csw;
		}
		if (cbw.bmCBWFlags.direction == USB_CBW_DIRECTION_OUT && cbw.dCBWDataTransferLength>0){
			debugPrintln("drainOutPipe");
			drainOutPipe(cbw.dCBWDataTransferLength);
		}
		// The response of a device to a CBW that is not meaningful is not specified.
		USB_MSC_CSW csw;
		csw.dCSWSignature = USB_CSW_SIGNATURE;
		csw.dCSWTag = cbw.dCBWTag;
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength;
		csw.bCSWStatus = USB_CSW_STATUS_FAIL;
		USBDevice.send(bulkInEndpoint, &csw, USB_CSW_SIZE);
		return FAILURE;
	}

	if ( (cbw.dCBWDataTransferLength == 0) ||
		 (cbw.bmCBWFlags.direction == USB_CBW_DIRECTION_IN) ){
		// from the device to the host.
		if (isInEndpointHalt){
			//USBDevice.sendZlp(bulkInEndpoint);
			USBDevice.stallIn(bulkInEndpoint);
			return FAILURE;
		}
		return receiveInRequest();
	}
	else
	if (cbw.bmCBWFlags.direction == USB_CBW_DIRECTION_OUT){ // from the host to the device.
		if (isOutEndpointHalt){
			//USBDevice.sendZlp(bulkOutEndpoint);
			USBDevice.stallOut(bulkOutEndpoint);
			return FAILURE;
		}
		return receiveOutRequest();
	}

	return 0;
}

/*
uint32_t MSC_::sendBlock(){ // sends block to USB
	debugPrint(" MSC_::sendBlock()\n");
	return USB_Send(txEndpoint, blockData, BLOCK_DATA_SZ);
}
*/
//#endif /* if defined(USBCON) */
