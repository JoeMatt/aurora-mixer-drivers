/*	Copyright: 	© Copyright 2005 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	AuroraUSBMIDI.cpp
	
=============================================================================*/

#include "AuroraUSBMIDI.h"

// ^^^^^^^^^^^^^^^^^^ CUSTOMIZED ^^^^^^^^^^^^^^^^^^^^^^

// MAKE A NEW UUID FOR EVERY NEW DRIVER!
#define kFactoryUUID CFUUIDGetConstantUUIDWithBytes(NULL, 0xB9, 0x32, 0xA6, 0x3D, 0xA6, 0xAA, 0x46, 0x08, 0x9A, 0x21, 0x94, 0x14, 0xF4, 0x65, 0xF6, 0xBB)
// B932A63D-A6AA-4608-9A21-9414F465F6BB

#define kTheInterfaceToUse	2

#define kMyVendorID			0x0403
#define kMyProductID		0xA238
#define kMyNumPorts			2		// might vary this by model

// -------------------- FTDI 2232 Chip Implimenation Defines ---------------------------------------

#define kFTDILatencyTimer		2
#define kFTDIDivisor			54		/* 3,000,000 / 57,600 baud = 54 */
#define kFTDIReadTimeout		0
#define kFTDIWriteTimeout		0
#define kFTDIInTransferSize		64		
#define kFTDIOutTransferSize	64		/* must be multiple of 64		*/

// __________________________________________________________________________________________________

/* The system's identifier for our bundle */
#define appID CFSTR("com.Aurora.driver.224")

// Implementation of the factory function for this type.
extern "C" void *NewAuroraUSBMIDIDriver(CFAllocatorRef allocator, CFUUIDRef typeID);
extern "C" void *NewAuroraUSBMIDIDriver(CFAllocatorRef allocator, CFUUIDRef typeID) 
{
	// If correct type is being requested, allocate an
	// instance of TestType and return the IUnknown interface.
	if (CFEqual(typeID, kMIDIDriverTypeID)) {
		AuroraUSBMIDIDriver *result = new AuroraUSBMIDIDriver;
		return result->Self();
	} else {
		// If the requested type is incorrect, return NULL.
		return NULL;
	}
}

// __________________________________________________________________________________________________

AuroraUSBMIDIDriver::AuroraUSBMIDIDriver() :
	USBVendorMIDIDriver(kFactoryUUID)
{
	/* Set the colors to 0x00, 0x00, 0x00 */
	bzero(&colors, 3);
}

AuroraUSBMIDIDriver::~AuroraUSBMIDIDriver()
{
}

// __________________________________________________________________________________________________

bool		AuroraUSBMIDIDriver::MatchDevice(	USBDevice *		inUSBDevice)
{
	const IOUSBDeviceDescriptor * devDesc = inUSBDevice->GetDeviceDescriptor();
	if (USBToHostWord(devDesc->idVendor) == kMyVendorID) {
		UInt16 devProduct = USBToHostWord(devDesc->idProduct);
		if (devProduct == kMyProductID)
		{
			// Set values here before passing back to the OS X USB Carrier
			FT_STATUS ftStatus; 
			
			ftStatus = FT_SetVIDPID(kMyVendorID, kMyProductID);
			if( ftStatus != FT_OK )
			{
				DLog("Error setting VID/PID\n");

				return false;
			}
			
			ftStatus = FT_Open(0,&ftHandle);
			if( ftStatus != FT_OK )
			{
				DLog("Error in FT_Open() opening device\n");

				return false;
			}
			
			ftStatus = FT_SetLatencyTimer(ftHandle, kFTDILatencyTimer); 
			if( ftStatus != FT_OK )
			{
				DLog("Error setting Latency Timer\n");

				return false;
			}
			
			ftStatus = FT_SetDataCharacteristics(ftHandle,FT_BITS_8,FT_STOP_BITS_1,FT_PARITY_NONE);
			if( ftStatus != FT_OK )
			{
				DLog("Error in setting data characteristics\n");

				return false;
			}
			
			ftStatus = FT_SetDivisor(ftHandle, kFTDIDivisor);
			if( ftStatus != FT_OK )
			{
				DLog("Error in setting BAUD rate\n");

				return false;
			}
			
			ftStatus = FT_SetUSBParameters(ftHandle, kFTDIInTransferSize, kFTDIOutTransferSize); 
			if( ftStatus != FT_OK )
			{
				DLog("Error in setting USB parameters\n");

				return false;
			}
			
			ftStatus = FT_SetFlowControl(ftHandle, FT_FLOW_NONE, NULL, NULL);
			if( ftStatus != FT_OK )
			{
				DLog("Error in setting flow control\n");

				return false;
			}
			
			ftStatus = FT_SetTimeouts(ftHandle, kFTDIReadTimeout, kFTDIWriteTimeout);
			if( ftStatus != FT_OK )
			{
				DLog("Error in setting timeouts\n");

				return false;
			}
			
			ftStatus = FT_Purge(ftHandle, FT_PURGE_RX + FT_PURGE_TX);
			if( ftStatus != FT_OK )
			{
				DLog("Error clearing device buffers\n");

				return false;
			}
			
			//Close device in FTDI library to hand over to our USB loop
			FT_Close(ftHandle);
			
			// Init Aurora224 variables
			

			DLog("Set values successfully\n");
			return true;
		}
		
	}
	return false;
}

MIDIDeviceRef	AuroraUSBMIDIDriver::CreateDevice(	USBDevice *		inUSBDevice,
													USBInterface *	inUSBInterface)
{
	MIDIDeviceRef dev;
	MIDIEntityRef ent;
	//UInt16 devProduct = USBToHostWord(inUSBDevice->GetDeviceDescriptor()->idProduct);
	
	CFStringRef boxName = CFSTR("224 Mixer");
	MIDIDeviceCreate(Self(),
		boxName,
		CFSTR("Aurora"),	// manufacturer name
		boxName,
		&dev);
	
	// make entity for each port, with 1 source, 1 destination
	for (int port = 1; port <= kMyNumPorts; ++port) {
		char portname[64];
		if (kMyNumPorts > 1)
			sprintf(portname, "Port %d", port);
		else
			CFStringGetCString(boxName, portname, sizeof(portname), kCFStringEncodingMacRoman);

		CFStringRef str = CFStringCreateWithCString(NULL, portname, 0);
		MIDIDeviceAddEntity(dev, str, false, 1, 1, &ent);
		CFRelease(str);
	}

	return dev;
}

USBInterface *	AuroraUSBMIDIDriver::CreateInterface(USBMIDIDevice *device)
{
	USBInterface *intf = device->mUSBDevice->FindInterface(kTheInterfaceToUse, 0);
	return intf;
}

void		AuroraUSBMIDIDriver::StartInterface(USBMIDIDevice *usbmDev)
{
}

void		AuroraUSBMIDIDriver::StopInterface(USBMIDIDevice *usbmDev)
{
}

void		AuroraUSBMIDIDriver::HandleInput(USBMIDIDevice *usbmDev, MIDITimeStamp when, Byte *readBuf, ByteCount readBufSize)
{
	/**    individual packet of the form:                                 **/
	/**                   {0x55 0x55 CC VV}                               **/
	/**    where CC is the midi continuous controller (cc) value and VV   **/
	/**    is the 7-bit value.											  **/
	
	/**	!	USB <-> Host endianess needs to be respected.				! **/
	/**	!	64Bit compatability needs to be respected for Snow Leopard	! **/
	
	#ifdef DEBUG
	printf("Read %2d bytes: ", (uint) readBufSize);
	for(uint x=0; x< readBufSize; x++)
		printf("0x%hx ",readBuf[x]);
	printf("\n");
	#endif
	
	// Most likely this method will FAIL bcs it's not meant to handle erratic serial data, but hey, don't know for sure without a test unit for now - JM 
	USBMIDIHandleInput(usbmDev, when, readBuf, readBufSize);
}

ByteCount	AuroraUSBMIDIDriver::PrepareOutput(USBMIDIDevice *usbmDev, WriteQueue &writeQueue, 
				Byte *destBuf)
{
	/**		{0x55 0x55 RR GG BB}										**/
	/**		Where RR GG & BB are Red, Green, Blue values from 0 - 255	**/
	/**		We're reading CC 1 - 3 0xB0 as the header					**/
	// Initializer buffer
	char buffer[5];
	//	Set serial header
	buffer[0]	= 0x55;
	buffer[1]	= 0x55;
	
	// Copy current RGB values
	memcpy(&buffer[2], &colors, 3);
	
	WriteQueue::iterator wqit = writeQueue.begin();
	WriteQueueElem *wqe = &(*wqit);
	Byte *dataStart = wqe->packet.Data();
	Byte *src = dataStart + wqe->bytesSent;
	
	if( src[0] != 0xB0)		//Not CC msg on Chan 1
		return 0;
	int color = src[1];

	if( color <= 3)
		buffer[color+1] = src[2] * 2; //We're getting values 0 - 127 but the hardware wants 0 - 255

	IOReturn ioreturn;
	IOUSBInterfaceInterface ** intf = usbmDev->mUSBIntfIntf;

	*((uint16_t *) buffer ) = HostToUSBWord( *((uint16_t *) buffer ) );

	
	ioreturn = (*intf)->WritePipe(intf, 2, buffer, 4);
	//ioreturn = (*intf)->WritePipeAsync(intf, 2, buffer, strlen(buffer), NULL, (void *) usbmDev);
	
	if (ioreturn != kIOReturnSuccess)
	{
		printf("unable to do bulk write (%08x)\n", ioreturn);
		(void) (*intf)->USBInterfaceClose(intf);
		(void) (*intf)->Release(intf);
		return 0;
	}

	DLog("Wrote \"%4X\" (%ld bytes) to bulk endpoint\n", buffer[0], (UInt32) strlen(buffer));
	
	return (UInt32) strlen(buffer);
	//int n = USBMIDIPrepareOutput(usbmDev, writeQueue, destBuf, usbmDev->mOutPipe.mMaxPacketSize);
//	if (n < usbmDev->mOutPipe.mMaxPacketSize) {
//		memset(destBuf + n, 0, usbmDev->mOutPipe.mMaxPacketSize - n);
//	}
//	return usbmDev->mOutPipe.mMaxPacketSize;
}


#if 0
// _________________________________________________________________________________________
// USBMIDIDriverBase::USBMIDIPrepareOutput
//
// WriteQueue is an STL list of WriteQueueElem's to be transmitted, presumably containing
// at least one element.
// Fill one USB buffer, destBuf, with a size of bufSize, with outgoing data in USB-MIDI format.
// Return the number of bytes written.
ByteCount	USBMIDIDriverBase::USBMIDIPrepareOutput(	USBMIDIDevice *	usbmDev, 
														WriteQueue &	writeQueue, 			
														Byte *			destBuf, 
														ByteCount 		bufSize)
{
	Byte *dest = destBuf, *destend = dest + bufSize;
	
	while (!writeQueue.empty()) {		
		WriteQueue::iterator wqit = writeQueue.begin();
		WriteQueueElem *wqe = &(*wqit);
		Byte *dataStart = wqe->packet.Data();
		Byte *src = dataStart + wqe->bytesSent;
		Byte *srcend = dataStart + wqe->packet.Length();
		int srcLeft;

#if !CAN_USE_USB_UNPARSED_EVENTS
		// have to check to see if we have 1 or 2 bytes of dangling unsent sysex (won't contain F7)
		srcLeft = srcend - src;
		if (srcLeft < 3 && !(src[0] & 0x80)) {
			// advance to the next packet
			WriteQueueElem *dangler = wqe;
			if (++wqit == writeQueue.end())
				break;	// no more packets past the dangler
			wqe = &(*wqit);
			if (wqe->portNum == dangler->portNum) {
				// here we go, another packet to this destination; insert dangling bytes into front of it
				wqe->packet.PrependBytes(src, srcLeft);
				// now we can remove the dangler from the queue
				dangler->packet.Dispose();
				writeQueue.pop_front();
			}
			// now just start processing *this* packet
			dataStart = wqe->packet.Data();
			src = dataStart + wqe->bytesSent;
			srcend = dataStart + wqe->packet.mLength;
		}
#endif

		Byte cableNibble = wqe->portNum << 4;

		while ((srcLeft = srcend - src) > 0 && dest <= destend - 4) {
			Byte c = *src++;
			
			switch (c >> 4) {
			case 0x0: case 0x1: case 0x2: case 0x3:
			case 0x4: case 0x5: case 0x6: case 0x7:
				// data byte, presumably a sysex continuation
				// F0 is also handled the same way
inSysEx:
				--srcLeft;
				if (srcLeft < 2 && (
					srcLeft == 0 
				|| (srcLeft == 1 && src[0] != 0xF7))) {
					// we don't have 3 sysex bytes to fill the packet with
#if CAN_USE_USB_UNPARSED_EVENTS
					// so we have to revert to non-parsed mode
					*dest++ = cableNibble | 0xF;
					*dest++ = c;
					*dest++ = 0;
					*dest++ = 0;
#else
					// undo consumption of the first source byte
					--src;
					wqe->bytesSent = src - dataStart;
					goto DoneFilling;
#endif
				} else {
					dest[1] = c;
					if ((dest[2] = *src++) == 0xF7) {
						dest[0] = cableNibble | 6;		// sysex ends with following 2 bytes
						dest[3] = 0;
					} else if ((dest[3] = *src++) == 0xF7)
						dest[0] = cableNibble | 7;		// sysex ends with following 3 bytes
					else
						dest[0] = cableNibble | 4;		// sysex continues
					dest += 4;
				}
				break;
			case 0x8:	// note-off
			case 0x9:	// note-on
			case 0xA:	// poly pressure
			case 0xB:	// control change
			case 0xE:	// pitch bend
				*dest++ = cableNibble | (c >> 4);
				*dest++ = c;
				*dest++ = *src++;
				*dest++ = *src++;
				break;
			case 0xC:	// program change
			case 0xD:	// mono pressure
				*dest++ = cableNibble | (c >> 4);
				*dest++ = c;
				*dest++ = *src++;
				*dest++ = 0;
				break;
			case 0xF:	// system message
				switch (c) {
				case 0xF0:	// sysex start
					goto inSysEx;
				case 0xF8:	// clock
				case 0xFA:	// start
				case 0xFB:	// continue
				case 0xFC:	// stop
				case 0xFE:	// active sensing
				case 0xFF:	// system reset
					*dest++ = cableNibble | 0xF;// 1-byte system realtime
					*dest++ = c;
					*dest++ = 0;
					*dest++ = 0;
					break;
				case 0xF6:	// tune request (0)
				case 0xF7:	// EOX
					*dest++ = cableNibble | 5;	// 1-byte system common or sysex ends with one byte
					*dest++ = c;
					*dest++ = 0;
					*dest++ = 0;
					break;
				case 0xF1:	// MTC (1)
				case 0xF3:	// song select (1)
					*dest++ = cableNibble | 2;	// 2-byte system common
					*dest++ = c;
					*dest++ = *src++;
					*dest++ = 0;
					break;
				case 0xF2:	// song pointer (2)
					*dest++ = cableNibble | 3;	// 3-byte system common
					*dest++ = c;
					*dest++ = *src++;
					*dest++ = *src++;
					break;
				default:
					// unknown MIDI message! advance until we find a status byte
					while (src < srcend && *src < 0x80)
						++src;
					break;
				}
				break;
			}
		
			if (src >= srcend) {
				// source packet completely sent
				wqe->packet.Dispose();
				writeQueue.erase(wqit);
				if (writeQueue.empty())
					break;
			} else
				wqe->bytesSent = src - dataStart;
		} // ran out of source data or filled buffer
#if !CAN_USE_USB_UNPARSED_EVENTS
DoneFilling:
#endif
		
		if (dest > destend - 4)
			// destBuf completely filled
			break;

		// we didn't fill the output buffer, loop around to look for more 
		// source data in the write queue
		
	} // while walking writeQueue
	return dest - destBuf;
}

#endif
