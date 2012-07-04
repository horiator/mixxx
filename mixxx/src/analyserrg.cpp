#include <QtDebug>
#include <time.h>
#include <math.h>

#include "sampleutil.h"
#include "trackinfoobject.h"
#include "playerinfo.h"
#include "analyserrg.h"
#include "../lib/replaygain/replaygain.h"

AnalyserGain::AnalyserGain(ConfigObject<ConfigValue> *_config) {
    m_pConfigReplayGain = _config;
    m_bStepControl = false;
    m_pLeftTempBuffer = NULL;
    m_pRightTempBuffer = NULL;
    m_iBufferSize = 0;
    m_pReplayGain = new ReplayGain();
}

AnalyserGain::~AnalyserGain() {
    delete [] m_pLeftTempBuffer;
    delete [] m_pRightTempBuffer;
    delete m_pReplayGain;
}

bool AnalyserGain::initialise(TrackPointer tio, int sampleRate, int totalSamples) {
    bool bAnalyserEnabled = (bool)m_pConfigReplayGain->getValueString(ConfigKey("[ReplayGain]","ReplayGainAnalyserEnabled")).toInt();
    float fReplayGain = tio->getReplayGain();
    if(totalSamples == 0 || fReplayGain != 0 || !bAnalyserEnabled) {
        //qDebug() << "Replaygain Analyser will not start.";
        //if (fReplayGain != 0 ) qDebug() << "Found a ReplayGain value of " << 20*log10(fReplayGain) << "dB for track :" <<(tio->getFilename());
        return false;
    }
    m_bStepControl = m_pReplayGain->initialise((long)sampleRate, 2);
    return true;
}

void AnalyserGain::cleanup(TrackPointer tio) {
    m_bStepControl = false;
    Q_UNUSED(tio);
}

void AnalyserGain::process(const CSAMPLE *pIn, const int iLen) {
    if(!m_bStepControl)
        return;

    int halfLength = static_cast<int>(iLen / 2);
    if (halfLength > m_iBufferSize) {
        delete [] m_pLeftTempBuffer;
        delete [] m_pRightTempBuffer;
        m_pLeftTempBuffer = new CSAMPLE[halfLength];
        m_pRightTempBuffer = new CSAMPLE[halfLength];
    }
    SampleUtil::deinterleaveBuffer(m_pLeftTempBuffer, m_pRightTempBuffer, pIn, halfLength);
    SampleUtil::applyGain(m_pLeftTempBuffer, 32767, halfLength);
    SampleUtil::applyGain(m_pRightTempBuffer, 32767, halfLength);
    m_bStepControl = m_pReplayGain->process(m_pLeftTempBuffer, m_pRightTempBuffer, halfLength);
}

void AnalyserGain::finalise(TrackPointer tio) {
    //TODO: We are going to store values as relative peaks so that "0" means that no replaygain has been evaluated.
    // This means that we are going to transform from dB to peaks and viceversa.
    // One may think to digg into replay_gain code and modify it so that
    // it directly sends results as relative peaks.
    // In that way there is no need to spend resources in calculating log10 or pow.
    if(!m_bStepControl)
        return;

    float ReplayGainOutput = m_pReplayGain->end();
    if (ReplayGainOutput == GAIN_NOT_ENOUGH_SAMPLES) {
        qDebug() << "ReplayGain analysis failed:" << ReplayGainOutput;
        m_bStepControl = false;
        return;
    }

    float fReplayGain_Result = pow(10,(ReplayGainOutput)/20);

    //qDebug() << "ReplayGain result is" << ReplayGainOutput << "pow:" << fReplayGain_Result;
    //qDebug()<<"ReplayGain outputs "<< ReplayGainOutput << "db for track "<< tio->getFilename();
    tio->setReplayGain(fReplayGain_Result);
    const PlayerInfo& info = PlayerInfo::Instance();
    if (info.isTrackLoaded(tio)) {
        // Wait until new replay gain is
        // set in enginepregain
        sleep(1);
    }
    //if(fReplayGain_Result) qDebug() << "ReplayGain Analyser found a ReplayGain value of "<< 20*log10(fReplayGain_Result) << "dB for track " << (tio->getFilename());
    m_bStepControl=false;
    //m_iStartTime = clock() - m_iStartTime;
    //qDebug() << "AnalyserGain :: Generation took " << double(m_iStartTime) / CLOCKS_PER_SEC << " seconds";
}
