/********************************************************
 ** 
 **  BasePeripheral.h
 ** 
 **  Part of the Makernet framework by Jeremy Gilbert
 ** 
 **  License: GPL 3
 **  See footer for copyright and license details.
 ** 
 ********************************************************/


#ifndef DATALINK_H
#define DATALINK_H

// The datalink layer handles putting bytes on wires with no knowledge of what
// those bytes mean. Each collection of bytes is called a "frame". The
// datalink interface is made interchangable so that multiple datalinks such
// as I2C, RFM, and even ethernet could all be makernet enabled.


#define MAX_MAKERNET_FRAME_LENGTH 255

typedef void (*frameReceiveCallback_t)( uint8_t *buffer, uint8_t readSize );

class Datalink {

public:
	// Start the datalink including any external peripherals
	virtual void initialize() = 0;
	// Send a single frame
	virtual int sendFrame( uint8_t *inBuffer, uint8_t len ) = 0;

	uint8_t frameBuffer[MAX_MAKERNET_FRAME_LENGTH];
	uint8_t address;

};

#endif
