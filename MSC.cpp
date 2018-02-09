/*
 * PluggableUSBMSCModule.cpp
 *
 *  Created on: Jan 27, 2018
 *      Author: mark
 */

#include <stdint.h>
#include <Arduino.h>
#include "MSC.h"
#include "SCSI.h"
#include <USB/PluggableUSB.h>
#include "my_debug.h"
//#include <USB/CDC.cpp>
#include "SCSIDevice.h"
#include "LcdConsole.h"

//#if defined(USBCON)
//byte blockData[MSC_BLOCK_DATA_SZ];

//String debug="BEGIN\n";

void blink(uint ms){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
}


MSC_::MSC_() : PluggableUSBModule(NUM_ENDPOINTS, NUM_INTERFACE, epType)
				   //descriptorSize(0),
                   //protocol(MSC_REPORT_PROTOCOL),
				   //idle(0)
{
	debug += "Create MSC\n";
	rxEndpoint = 0;
	txEndpoint = 0;
	epType[0] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0); //tx
	epType[1] =  USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0); //rx
	data = NULL;
	PluggableUSB().plug(this);
}

MSC_::~MSC_(void){}

int MSC_::getInterface(uint8_t* interfaceCount)
{
	debug += "MSC_::getInterface\n";

	*interfaceCount += 1; // uses 1

	MSCDescriptor MSCInterface = {
		D_INTERFACE(pluggedInterface, NUM_ENDPOINTS, USB_DEVICE_CLASS_STORAGE,
				MSC_SUBCLASS_SCSI, MSC_PROTOCOL_BULK_ONLY),
		D_ENDPOINT(USB_ENDPOINT_OUT(MSC_ENDPOINT_OUT),
				USB_ENDPOINT_TYPE_BULK, MSC_BLOCK_DATA_SZ, 0),
		D_ENDPOINT(USB_ENDPOINT_IN (MSC_ENDPOINT_IN),
				USB_ENDPOINT_TYPE_BULK, MSC_BLOCK_DATA_SZ, 0)
	};

	rxEndpoint = MSC_ENDPOINT_OUT;
	txEndpoint = MSC_ENDPOINT_IN;
	return USB_SendControl(0, &MSCInterface, sizeof(MSCInterface));
}

int MSC_::getDescriptor(USBSetup& setup)
{
	debug += "MSC_::getDescriptor\n";
	debug += " setup.bRequest:"+String(setup.bRequest)+"\n";
	debug += " setup.bmRequestType:"+String(setup.bmRequestType)+"\n";

	if (setup.bmRequestType == REQUEST_DEVICETOHOST) {
		debug += "   setup.bmRequestType == REQUEST_DEVICETOHOST\n";
		debug += "   setup.wIndex: "+String(setup.wIndex)+"\n";
		debug += "   setup.wLength: "+String(setup.wLength)+"\n";
		debug += "   setup.wValueL: "+String(setup.wValueL)+"\n";
		debug += "   setup.wValueH: "+String(setup.wValueH)+"\n";
		return 0;
	}

	// In a MSC Class Descriptor wIndex contains the interface number
	if (setup.wIndex != pluggedInterface) { return 0; }


	// Check if this is a MSC Class Descriptor request
	if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) {
		debug += "   setup.bmRequestType == REQUEST_DEVICETOHOST_STANDARD_INTERFACE\n";
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

uint8_t MSC_::getShortName(char *name)
{
	debug += "MSC_::getShortName\n";
	memcpy(name, "CDC,MSC", 7);
	return 7;
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

uint8_t maxlun = 0;

// it is required for MSC, host driver needs to set LUN etc...
bool MSC_::setup(USBSetup& setup)
{
	debug += "MSC_::setup\n";

	if (pluggedInterface != setup.wIndex) {
		return false;
	}
	debug += " setup.bRequest:"+String(setup.bRequest)+"\n";
	debug += " setup.bmRequestType:"+String(setup.bmRequestType)+"\n";

	uint8_t request = setup.bRequest;
	uint8_t requestType = setup.bmRequestType;
	//uint16_t length = setup.wLength;
	//uint16_t value = setup.wValueL + (setup.wValueH << 8);
	//uint16_t index = setup.wIndex;

	if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE)
	{
		debug += "   requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE\n";
		debug += "   request:" + String(request) + " 0x"+String(request)+"\n";
		if (request == MSC_RESET) {
			debug += "     MSC_RESET\n";
			reset();
			return true;
		}
		if (request == MSC_GET_MAX_LUN) {
			debug += "     MSC_GET_MAX_LUN\n";
			int r = USB_SendControl(pluggedEndpoint, &maxlun, 1);
			debug += "   r:"+String(r); debug += "\n";
			return true;
		}
		if (request == MSC_SUBCLASS_SCSI) {
			debug += "     MSC_SUBCLASS_SCSI\n";
			return true;
		}
	}

	if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE)
	{
		debug += "   requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE\n";
		debug += "   request:" + String(request) + " 0x"+String(request)+"\n";
	}

	return false;
}

bool MSC_::reset(){
	//Serial.println("reset");
	// TODO actual reset
	return true;
}

void debugCBW(USB_MSC_CBW &cbw){
	debug += "      cbw.dCBWSignature:"+String(cbw.dCBWSignature)+"\n";
	debug += "      cbw.dCBWTag:"+String(cbw.dCBWTag)+"\n";
	debug += "      cbw.dCBWDataTransferLength:"+String(cbw.dCBWDataTransferLength)+"\n";
	debug += "      cbw.bmCBWFlags:"+String(cbw.bmCBWFlags)
			+ ((cbw.bmCBWFlags==USB_CBW_DIRECTION_IN)?"(IN)":
					((cbw.bmCBWFlags==0)?"(OUT)":"(BAD)"))
			+"\n";
	debug += "      cbw.bCBWLUN:"+String(cbw.bCBWLUN)+"\n";
	debug += "      cbw.bCBWCBLength:"+String(cbw.bCBWCBLength)+"\n";
	debug += "      cbw.CBWCB:";
	for (int i=0; i<cbw.bCBWCBLength; i++){
		debug +=String(cbw.CBWCB[i])+",";
	}
	debug += "\n";
}

/*
void debugPrintRespose(const SCSI_STANDARD_INQUIRY_DATA *inc){
	debugPrintlnSC("    inc.t10_vendor_id:", (char*)inc->inquiry.t10_vendor_id, 8);
	debugPrintlnSC("    inquiryData.inquiry.product_id:", (char*)inc->inquiry.product_id, 16);
}
*/

/*
 * Data-In - Indicates a transfer of data IN from the device to the host.
 */
uint32_t MSC_::receiveInBlock(){
	lcdConsole.println("IN:"+ String(cbw.dCBWDataTransferLength));
	lcdConsole.refresh();

	//debug+="USB_CBW_DIRECTION_IN: len:" + String(cbw.dCBWDataTransferLength)+"\n";
	//uint16_t rlen = cbw.dCBWDataTransferLength;
	uint32_t tfLen = cbw.dCBWDataTransferLength;

	//uint8_t* response = NULL;
	//if (rlen) response = (uint8_t*)malloc(rlen);
	//debugPrintlnSX("  cbw.CBWCB:",cbw.CBWCB, cbw.bCBWCBLength);

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);
	//debug+=" cbd.read10.LBA:"+String(cbd.read10.LBA)+" cbd.read10.length:"+String(cbd.read10.length)+"\n";

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;

	int rlen = scsiDev.processRequest(cbd, data, tfLen);
	uint32_t slen = 0;
	while (rlen>0){
		rlen = scsiDev.readData(data);
		lcdConsole.println("send len:"+ String(rlen));
		slen += USBDevice.send(txEndpoint, data, rlen);
	}
	//USBDevice.flush(txEndpoint);

	/* For Data-In the device shall report in the dCSWDataResidue
	 * the difference between the amount of data expected as stated
	 * in the dCBWDataTransferLength and the actual amount of relevant
	 * data sent by the device. */
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength - slen;
	csw.bCSWStatus = USB_CSW_STATUS_PASS; // TODO error handling

	//debug+="  USB_Send CSW\n";
	//Serial.print(debug); debug="";
	USBDevice.send(txEndpoint, &csw, USB_CSW_SIZE);
	//debug+="  USB_Sent CSW\n";
	//Serial.print(debug); debug="";
	USBDevice.flush(txEndpoint);

	return slen;
}

/*
 * Data-Out Indicates a transfer of data OUT from the host to the device.
 */
uint32_t MSC_::receiveOutBlock(){ // receives block from USB
	lcdConsole.println("OUT:"+ String(cbw.dCBWDataTransferLength));
	lcdConsole.refresh();

	//debug+="USB_CBW_DIRECTION_IN: len:" + String(cbw.dCBWDataTransferLength)+"\n";
	//uint16_t rlen = cbw.dCBWDataTransferLength;
	uint32_t tfLen = cbw.dCBWDataTransferLength;

	//uint8_t* response = NULL;
	//if (rlen) response = (uint8_t*)malloc(rlen);
	//debugPrintlnSX("  cbw.CBWCB:",cbw.CBWCB, cbw.bCBWCBLength);

	SCSI_CBD cbd;
	memcpy(cbd.array, cbw.CBWCB, cbw.bCBWCBLength);
	//debug+=" cbd.read10.LBA:"+String(cbd.read10.LBA)+" cbd.read10.length:"+String(cbd.read10.length)+"\n";

	USB_MSC_CSW csw;
	csw.dCSWSignature = USB_CSW_SIGNATURE;
	csw.dCSWTag = cbw.dCBWTag;

	int rlen = scsiDev.processRequest(cbd, data, tfLen);
	uint32_t slen = 0;
	while (rlen>0){
		rlen = scsiDev.readData(data);
		lcdConsole.println("send len:"+ String(rlen));
		slen += USBDevice.send(txEndpoint, data, rlen);
	}
	//USBDevice.flush(txEndpoint);

	/* For Data-In the device shall report in the dCSWDataResidue
	 * the difference between the amount of data expected as stated
	 * in the dCBWDataTransferLength and the actual amount of relevant
	 * data sent by the device. */
	csw.dCSWDataResidue = cbw.dCBWDataTransferLength - slen;
	csw.bCSWStatus = USB_CSW_STATUS_PASS; // TODO error handling

	//debug+="  USB_Send CSW\n";
	//Serial.print(debug); debug="";
	USBDevice.send(txEndpoint, &csw, USB_CSW_SIZE);
	//debug+="  USB_Sent CSW\n";
	//Serial.print(debug); debug="";
	USBDevice.flush(txEndpoint);

	return slen;
}

uint32_t MSC_::receiveBlock(){ // receives block from USB
	//debug += "MSC_::receiveBlock()";
	//Serial.print(debug); debug="";
	uint32_t rxa = USBDevice.available(rxEndpoint);
	//debug += "  USBDevice.available rx:"+String(rxa)+"\n";
	//Serial.print(debug); debug="";

	if (rxa >= USB_CBW_SIZE) {
		int r = USB_Recv(rxEndpoint, &cbw, USB_CBW_SIZE);
		if (r > 0) debug += " r:"+String(r)+"\n";
		if (r==31 && cbw.dCBWSignature == USB_CBW_SIGNATURE){
			//debugCBW(cbw);
			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_IN){ // from the device to the host.
				receiveInBlock();
			} else
			if (cbw.bmCBWFlags==USB_CBW_DIRECTION_OUT){ // from the host to the device.
				receiveOutBlock();
			}
		}
		return r;
	}
	else {
		debug += "\n";
		return 0;
	}
}

/*
uint32_t MSC_::sendBlock(){ // sends block to USB
	debug += " MSC_::sendBlock()\n";
	return USB_Send(txEndpoint, blockData, BLOCK_DATA_SZ);
}
*/
//#endif /* if defined(USBCON) */
