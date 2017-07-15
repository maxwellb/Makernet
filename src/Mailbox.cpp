/********************************************************
 **
 **  Mailbox.cpp
 **
 **  Part of the Makernet framework by Jeremy Gilbert
 **
 **  License: GPL 3
 **  See footer for copyright and license details.
 **
 ********************************************************/

#include <Mailbox.h>
#include <Types.h>
#include <Debug.h>
#include <Util.h>
#include <Globals.h>
#include <Makernet.h>

#include <strings.h>

void Mailbox::busReset()
{

}

SmallMailbox::SmallMailbox(uint8_t configFlags, const char *d)
{
	contents = (uint8_t *)(void *)&__contents;
	flags = configFlags;
	size = sizeof( __contents );
	description = d;
	if ( contents != NULL and size > 0 )
		memset( contents, 0, size );
	synchronized = 1;
	callerChanged = 0;
}

void SmallMailbox::busReset()
{
	// Bus reset should force all caller changed information to be re-sent
	// over the network.
	retryTimer.trigger();
	if ( callerChanged )
		synchronized = 0;
	else
		synchronized = 1;

	changeTrigger = 0;

	DPF( dMAILBOX | dRESET, "&&&& RESET: [%s] - sync=%d callerChanged=%d value=%d\n",
	     description, synchronized, callerChanged, __contents  );
}

int SmallMailbox::hasPendingChanges()
{
	if ( !retryTimer.ready() )
		return 0;

	if ( !synchronized ) {
		// DPF( dMAILBOX, "&&&& HPC??: [%s] - sync=%d callerChanged=%d value=%d\n",
		//      description, synchronized, callerChanged, __contents  );
	}

	return !synchronized;
}

struct SmallMailboxMessage {
	enum class Command : uint8_t { SEND_VALUE, ACK_VALUE, SEND_VALUE_CHANGE } command;
	uint32_t value;
} __attribute__((packed)); // magic to prevent compiler from aligning contents of struct

// Called by the framework when we've indicated a pending message is
// available. Our job is to transmit any unacknowledged values

int SmallMailbox::generateMessage( uint8_t *buffer, int size )
{
	if (API_CHECK and ( size < sizeof( SmallMailboxMessage ) ))
		return -1;
	if (API_CHECK and ( buffer == NULL ))
		return -2;

	auto *msg = reinterpret_cast<SmallMailboxMessage *>(buffer);

	// Change trigger generates a one-shot message with the CHANGE flag on.
	// Future value updates or synhronizations will just be SEND_VALUE.
	// On ACK, we clear the changeTrigger.

	if ( changeTrigger && callerChanged )
		msg->command = SmallMailboxMessage::Command::SEND_VALUE_CHANGE;
	else
		msg->command = SmallMailboxMessage::Command::SEND_VALUE;

	msg->value = __contents;

	retryTimer.reset();

	return sizeof( SmallMailboxMessage );
}

int SmallMailbox::handleMessage( uint8_t *buffer, int size )
{
	if (API_CHECK and ( size < sizeof( SmallMailboxMessage ) ))
		return -1;
	if (API_CHECK and ( buffer == NULL ))
		return -2;

	auto *msg = reinterpret_cast<SmallMailboxMessage *>(buffer);

	if ( msg->command == SmallMailboxMessage::Command::SEND_VALUE or
	        msg->command == SmallMailboxMessage::Command::SEND_VALUE_CHANGE ) {
		if ( callerChanged ) {
			// Contention case. Here both sides have updated the mailbox.
			DPR( dMAILBOX, "&&&& Contention: Incoming mailbox push when callerChanged=1");
			DLN( dMAILBOX );

			if ( Makernet.network.role == Network::master ) {
				// In the contention case, the master overides. We ack receipt of the value even though
				// we don't accept it. And we trigger a synchronization on our end.
				DLN( dMAILBOX, "&&&& Contention: Disregarding incoming value");
				synchronized = 0;
				msg->command = SmallMailboxMessage::Command::ACK_VALUE;
				return (sizeof( SmallMailboxMessage ));
			} else {
				DLN( dMAILBOX, "&&&& Contention: proceeding with normal update that overides our value");
			}
		}

		// A normal update - accept the packet's data, mark internally as
		// synchronized, and clear caller-changed flag
		__contents = msg->value;

		synchronized = 1;
		callerChanged = 0;

		DPR( dMAILBOX, "&&&& Mailbox value recv: [");
		DPR( dMAILBOX, description );
		DPR( dMAILBOX, "] updated over network to: [");
		hexPrint( dMAILBOX, reinterpret_cast<uint8_t *>(contents), 4 );
		DPR( dMAILBOX, "] as ui32: [");
		DPR( dMAILBOX, __contents );
		DLN( dMAILBOX, "]");

		// The _CHANGE varient is triggered once when a caller has issued the changes
		// as opposed to a value being sent over during a synchronization. As soon
		// as we dispatch the event, changeTrigger should be made zero.

		if ( msg->command == SmallMailboxMessage::Command::SEND_VALUE_CHANGE )
			changeTrigger = 1;

		if ( observer != NULL )
			observer->onMailboxChange( this, changeTrigger );

		changeTrigger = 0;

		// Reformat the message and send it back as an ACK
		msg->command = SmallMailboxMessage::Command::ACK_VALUE;

		return (sizeof( SmallMailboxMessage ));
	}

	if ( msg->command == SmallMailboxMessage::Command::ACK_VALUE ) {
		// If the ACK does not contain the lastest value, remain
		// unsynchronized so the update will occur on the next pass
		if ( msg->value == __contents )
			synchronized = 1;
		else
			DLN( dMAILBOX | dWARNING, "&&&& Mailbox ACK incorrect, not clearing sync flag" );

		changeTrigger = 0;

		DPR( dMAILBOX, "&&&& Mailbox value acknowledgement: [");
		DPR( dMAILBOX, description );
		DPR( dMAILBOX, "] updated over network to: [");
		hexPrint( dMAILBOX, reinterpret_cast<uint8_t *>(contents), 4 );
		DPR( dMAILBOX, "] as ui32: [");
		DPR( dMAILBOX, __contents );
		DLN( dMAILBOX, "]");

		return 0;
	}



	return 1;
}

void SmallMailbox::trigger()
{
	synchronized = 0;
	callerChanged = 1;
	changeTrigger = 1;
	retryTimer.trigger();
}

void SmallMailbox::setLong( uint32_t v )
{
	__contents = v;

	retryTimer.trigger();

	synchronized = 0;
	callerChanged = 1;
	changeTrigger = 1;

	DPR( dMAILBOX | dMAILBOXVALUES, "&&&& MailboxChange: [");
	DPR( dMAILBOX | dMAILBOXVALUES, description );
	DPR( dMAILBOX | dMAILBOXVALUES, "] set to: [");
	hexPrint( dMAILBOX | dMAILBOXVALUES, contents, 4 );
	DPR( dMAILBOX | dMAILBOXVALUES, "] as long: [");
	DPR( dMAILBOX | dMAILBOXVALUES, __contents );
	DLN( dMAILBOX | dMAILBOXVALUES, "]");
}

uint32_t SmallMailbox::getLong()
{
	return ( __contents );
}

int32_t SmallMailbox::getLongSigned()
{
	return ( (signed long)__contents );
}



// Consider forking this

void SmallMailbox::enqueueEvent( KeyEvent kv )
{
	uint32_t tmp;
	memcpy( &tmp, &kv, sizeof( tmp ));
	setLong( tmp );
}

KeyEvent *SmallMailbox::getValueAsKeyEventPtr()
{
	return (KeyEvent *)contents;
}
