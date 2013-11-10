
#if defined(__APPLE__)

#elif defined(__WINDOWS__)

#else

#include "lbcontrolleralsa.h"


#include <QDebug>

LbControllerAlsa::LbControllerAlsa()
    : m_pAlsaSeq(NULL),
      m_iAlsaClient(-1),
      m_portNrIn(-1),
      m_portNrOut(-1) {
    openClient("Mixxx");
    registerInputDevice("Mixxx");
    registerOutputDevice("Mixxx");
}

LbControllerAlsa::~LbControllerAlsa() {
    closeClient();
}

bool LbControllerAlsa::openClient(const QString& name) {
    // Open new ALSA sequencer client...
//    if (snd_seq_open(&m_pAlsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
//        return false;
//    }

    // Set client identification...
    const QByteArray aClientName = name.toLocal8Bit();
//    snd_seq_set_client_name(m_pAlsaSeq, aClientName.constData());
//    m_iAlsaClient = snd_seq_client_id(m_pAlsaSeq);

    return true;
}

bool LbControllerAlsa::registerInputDevice(const QString& name) {
    if (m_portNrIn < 0 ) {
//        snd_seq_port_info_t *pinfo;
//        snd_seq_port_info_alloca(&pinfo);
//        snd_seq_port_info_set_capability(pinfo,
//                SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
//        snd_seq_port_info_set_type(pinfo,
//                SND_SEQ_PORT_TYPE_MIDI_GENERIC |SND_SEQ_PORT_TYPE_APPLICATION);
//        snd_seq_port_info_set_midi_channels(pinfo, 16);
//        snd_seq_port_info_set_timestamping(pinfo, 1);
//        snd_seq_port_info_set_timestamp_real(pinfo, 1);
//        snd_seq_port_info_set_timestamp_queue(pinfo, data->queue_id);
//        snd_seq_port_info_set_name(pinfo, name.toLocal8Bit());
//        m_portNrIn = snd_seq_create_port(m_pAlsaSeq, pinfo);

        if ( m_portNrIn < 0 ) {
            qDebug() << "LbControllerAlsa::registerOutputDevice: error register an output device";
            return false;
        }
        return true;
    }
    return false;
}

bool LbControllerAlsa::registerOutputDevice(const QString& name) {
    if (m_portNrOut < 0 ) {
 //       m_portNrOut = snd_seq_create_simple_port(m_pAlsaSeq, name.toLocal8Bit(),
 //               SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
 //               SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

        if (m_portNrOut < 0 ) {
            qDebug() << "LbControllerAlsa::registerOutputDevice: error register an output device";
            return false;
        }
        return true;
    }
    return false;
}


bool LbControllerAlsa::closeClient() {
    if (m_pAlsaSeq) {
 //       snd_seq_close(m_pAlsaSeq);
        m_iAlsaClient = -1;
        m_pAlsaSeq = NULL;
    }
    return true;
}


#endif

