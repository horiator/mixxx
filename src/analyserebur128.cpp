#include <QtDebug>
#include <ebu_r128_proc.h>

#include "sampleutil.h"
#include "trackinfoobject.h"
#include "analyserebur128.h"
#include "util/math.h"

static const float kReplayGain2ReferenceLUFS = -18;

AnalyserEbur128::AnalyserEbur128(ConfigObject<ConfigValue> *config)
        : m_pConfig(config),
          m_initalized(false),
          m_iBufferSize(0) {
    m_pTempBuffer[0] = NULL;
    m_pTempBuffer[1] = NULL;
    m_pEbu128Proc = new Ebu_r128_proc();
}

AnalyserEbur128::~AnalyserEbur128() {
    delete [] m_pTempBuffer[0];
    delete [] m_pTempBuffer[1];
    delete m_pEbu128Proc;
}

bool AnalyserEbur128::initialise(TrackPointer tio, int sampleRate, int totalSamples) {
    if (loadStored(tio) || totalSamples == 0) {
        return false;
    }

    m_pEbu128Proc->init (2, sampleRate);
    m_pEbu128Proc->integr_start ();
    m_initalized = true;
    return true;
}

bool AnalyserEbur128::loadStored(TrackPointer tio) const {
    bool bAnalyserEnabled = (bool)m_pConfig->getValueString(
            ConfigKey("[ReplayGain]","ReplayGainAnalyserEnabled")).toInt();
    float fReplayGain = tio->getReplayGain();
    if (/*fReplayGain != 0 ||*/ !bAnalyserEnabled) {
        return true;
    }
    return false;
}

void AnalyserEbur128::cleanup(TrackPointer tio) {
    Q_UNUSED(tio);
    m_pEbu128Proc->reset();
    m_initalized = false;
}

void AnalyserEbur128::process(const CSAMPLE *pIn, const int iLen) {
    if (!m_initalized) {
        return;
    }
    int halfLength = static_cast<int>(iLen / 2);
    if (halfLength > m_iBufferSize) {
        delete [] m_pTempBuffer[0];
        delete [] m_pTempBuffer[1];
        m_pTempBuffer[0] = new CSAMPLE[halfLength];
        m_pTempBuffer[1] = new CSAMPLE[halfLength];
    }
    SampleUtil::deinterleaveBuffer(m_pTempBuffer[0], m_pTempBuffer[1], pIn, halfLength);
    m_pEbu128Proc->process(halfLength, m_pTempBuffer);
}

void AnalyserEbur128::finalise(TrackPointer tio) {
    if (!m_initalized) {
        return;
    }
    const float averageLufs = m_pEbu128Proc->integrated();
    const float fReplayGain2 = db2ratio(kReplayGain2ReferenceLUFS - averageLufs);
    qDebug() << "ReplayGain2 result is" << averageLufs << "LUFS RG2:" << fReplayGain2 << "for" << tio->getFilename();
    tio->setReplayGain(fReplayGain2);
}
