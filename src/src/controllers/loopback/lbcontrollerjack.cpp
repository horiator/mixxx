/*
 * LbControllerJack.cpp
 *
 *  Created on: 31.03.2013
 *      Author: daniel
 */

#include "LbControllerJack.h"

LbControllerJack::LbControllerJack() {
    openClient("Mixxx");
    registerInputDevice("Mixxx");
    registerOutputDevice("Mixxx");
}

LbControllerJack::~LbControllerJack() {
}

bool LbControllerJack::openClient(const QString& name) {

}

bool LbControllerJack::registerInputDevice(const QString& name) {

}

bool LbControllerJack::registerOutputDevice(const QString& name) {

}


bool LbControllerJack::closeClient() {
    return true;
}



void RtMidiIn :: openVirtualPort( const std::string portName )
{
  JackMidiData *data = static_cast<JackMidiData *> (apiData_);

  if ( data->port == NULL )
    data->port = jack_port_register( data->client, portName.c_str(),
                                     JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );

  if ( data->port == NULL ) {
    errorString_ = "RtMidiOut::openVirtualPort: JACK error creating virtual port";
    error( RtError::DRIVER_ERROR );
  }
}

void RtMidiOut :: openVirtualPort( const std::string portName )
{
  JackMidiData *data = static_cast<JackMidiData *> (apiData_);

  if ( data->port == NULL )
    data->port = jack_port_register( data->client, portName.c_str(),
      JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0 );

  if ( data->port == NULL ) {
    errorString_ = "RtMidiOut::openVirtualPort: JACK error creating virtual port";
    error( RtError::DRIVER_ERROR );
  }
}

// Device initialization method.
bool qmidinetJackMidiDevice::open ( const QString& sClientName, int iNumPorts )
{
    // Close if already open.
    close();

    // Open new JACK client...
    const QByteArray aClientName = sClientName.toLocal8Bit();
    m_pJackClient = jack_client_open(
        aClientName.constData(), JackNullOption, NULL);
    if (m_pJackClient == NULL)
        return false;

    m_nports = iNumPorts;

    int i;

    // Create duplex ports.
    m_ppJackPortIn  = new jack_port_t * [m_nports];
    m_ppJackPortOut = new jack_port_t * [m_nports];

    for (i = 0; i < m_nports; ++i) {
        m_ppJackPortIn[i] = NULL;
        m_ppJackPortOut[i] = NULL;
    }

    const QString sPortNameIn("in_%1");
    const QString sPortNameOut("out_%1");
    for (i = 0; i < m_nports; ++i) {
        m_ppJackPortIn[i] = jack_port_register(m_pJackClient,
            sPortNameIn.arg(i + 1).toLocal8Bit().constData(),
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsInput, 0);
        m_ppJackPortOut[i] = jack_port_register(m_pJackClient,
            sPortNameOut.arg(i + 1).toLocal8Bit().constData(),
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsOutput, 0);
    }

    // Create transient buffers.
    m_pJackBufferIn  = jack_ringbuffer_create(1024 * m_nports);
    m_pJackBufferOut = jack_ringbuffer_create(1024 * m_nports);

    // Prepare the queue sorter stuff...
    m_pQueueIn = new qmidinetJackMidiQueue(1024 * m_nports, 8);

    // Set and go usual callbacks...
    jack_set_process_callback(m_pJackClient,
        qmidinetJackMidiDevice_process, this);
    jack_on_shutdown(m_pJackClient,
        qmidinetJackMidiDevice_shutdown, this);

    jack_activate(m_pJackClient);

    // Start listener thread...
    m_pRecvThread = new qmidinetJackMidiThread();
    m_pRecvThread->start();

    // Done.
    return true;
}


// Device termination method.
void qmidinetJackMidiDevice::close (void)
{
    if (m_pRecvThread) {
        if (m_pRecvThread->isRunning()) do {
            m_pRecvThread->setRunState(false);
        //  m_pRecvThread->terminate();
            m_pRecvThread->sync();
        } while (!m_pRecvThread->wait(100));
        delete m_pRecvThread;
        m_pRecvThread = NULL;
    }

    if (m_pJackClient)
        jack_deactivate(m_pJackClient);

    if (m_ppJackPortIn || m_ppJackPortOut) {
        for (int i = 0; i < m_nports; ++i) {
            if (m_ppJackPortIn && m_ppJackPortIn[i])
                jack_port_unregister(m_pJackClient, m_ppJackPortIn[i]);
            if (m_ppJackPortOut && m_ppJackPortOut[i])
                jack_port_unregister(m_pJackClient, m_ppJackPortOut[i]);
        }
        if (m_ppJackPortIn)
            delete [] m_ppJackPortIn;
        if (m_ppJackPortOut)
            delete [] m_ppJackPortOut;
        m_ppJackPortIn = NULL;
        m_ppJackPortOut = NULL;
    }

    if (m_pJackClient) {
        jack_client_close(m_pJackClient);
        m_pJackClient = NULL;
    }

    if (m_pJackBufferIn) {
        jack_ringbuffer_free(m_pJackBufferIn);
        m_pJackBufferIn = NULL;
    }

    if (m_pJackBufferOut) {
        jack_ringbuffer_free(m_pJackBufferOut);
        m_pJackBufferOut = NULL;
    }

    if (m_pQueueIn) {
        delete m_pQueueIn;
        m_pQueueIn = NULL;
    }

    m_nports = 0;
}

