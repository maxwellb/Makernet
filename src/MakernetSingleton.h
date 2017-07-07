#ifndef MAKERNETSINGLETON_H
#define MAKERNETSINGLETON_H


// This is a core singleton for the entire application. Right now, as an
// optimization, the singleton is used inside the Makernet framework itself
// so that each object is not forced to carry around pointers to the
// framework objects like Network and Datalink.
//

#include <Interval.h>
#include <Packet.h>
#include <Network.h>

class _Makernet {
public:
	Interval reportingInterval = Interval(5000);
	Network network;
	DeviceType deviceType;
	uint16_t hardwareID;
	uint16_t generation;

	void initialize();
	void loop();
};

extern _Makernet Makernet;



#endif