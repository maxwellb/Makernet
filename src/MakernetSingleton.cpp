/********************************************************
 ** 
 **  MakernetSingleton.cpp
 ** 
 **  Part of the Makernet framework by Jeremy Gilbert
 ** 
 **  License: GPL 3
 **  See footer for copyright and license details.
 ** 
 ********************************************************/

#include <MakernetSingleton.h>
#include <Debug.h>
#include <ArduinoAPI.h>
#include <BasePeripheral.h>

// Framework users should call this once during program setup. 

void _Makernet::initialize()
{
	DLN( dOBJFRAMEWORK, "**** Makernet framework init");
	generation = getRandomNumber16();
	hardwareID = getHardwareID();
	network.initialize();
	BasePeripheral::initializeAllPeripherals();
}

// busReset() is a special verb that is intended to reset all state around the
// entire network. When this is called, every object should assume all
// information it has about other devices in the network is potentially
// wrong.. clear buffers, interrupt work in progress, and wait for new
// synchronization data to restore state. This is called exactly once at the
// very end of initialization (so its guarnenteed to be called at the start of
// the program), and then again when requested by the controller over the
// network.

void _Makernet::busReset()
{
	network.busReset();
	BasePeripheral::busResetAllPeripherals();
}

void _Makernet::loop()
{
	if ( reportingInterval.hasPassed() )
		DPF( dSTATUSMSG, "+++ STATUS +++ hwID[%d] type[%d] gen[%d] millis=[%u]\n", hardwareID, deviceType, generation, millis() );

	network.loop();
}



// Singleton object
_Makernet Makernet;

