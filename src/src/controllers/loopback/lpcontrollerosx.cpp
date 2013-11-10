/*
 * LpControllerOsx.cpp
 *
 *  Created on: 31.03.2013
 *      Author: daniel
 */

#include "LpControllerOsx.h"

LpControllerOsx::LpControllerOsx() {
    // TODO Auto-generated constructor stub

}

LpControllerOsx::~LpControllerOsx() {
    // TODO Auto-generated destructor stub
}


void RtMidiIn :: openVirtualPort( const std::string portName )
{
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);

  // Create a virtual MIDI input destination.
  MIDIEndpointRef endpoint;
  OSStatus result = MIDIDestinationCreate( data->client,
                                           CFStringCreateWithCString( NULL, portName.c_str(), kCFStringEncodingASCII ),
                                           midiInputCallback, (void *)&inputData_, &endpoint );
  if ( result != noErr ) {
    errorString_ = "RtMidiIn::openVirtualPort: error creating virtual OS-X MIDI destination.";
    error( RtError::DRIVER_ERROR );
  }

  // Save our api-specific connection information.
  data->endpoint = endpoint;
}

void RtMidiOut :: openVirtualPort( std::string portName )
{
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);

  if ( data->endpoint ) {
    errorString_ = "RtMidiOut::openVirtualPort: a virtual output port already exists!";
    error( RtError::WARNING );
    return;
  }

  // Create a virtual MIDI output source.
  MIDIEndpointRef endpoint;
  OSStatus result = MIDISourceCreate( data->client,
                                      CFStringCreateWithCString( NULL, portName.c_str(), kCFStringEncodingASCII ),
                                      &endpoint );
  if ( result != noErr ) {
    errorString_ = "RtMidiOut::initialize: error creating OS-X virtual MIDI source.";
    error( RtError::DRIVER_ERROR );
  }

  // Save our api-specific connection information.
  data->endpoint = endpoint;
}
