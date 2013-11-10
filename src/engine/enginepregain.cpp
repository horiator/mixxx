/***************************************************************************
                          enginepregain.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtDebug>

#include "engine/enginepregain.h"
#include "controllogpotmeter.h"
#include "controlpotmeter.h"
#include "controlpushbutton.h"
#include "configobject.h"
#include "controlobject.h"

#include "sampleutil.h"
#include <time.h>   // for clock() and CLOCKS_PER_SEC

ControlPotmeter* EnginePregain::s_pReplayGainBoost = NULL;
ControlObject* EnginePregain::s_pEnableReplayGain = NULL;

/*----------------------------------------------------------------
   A pregaincontrol is ... a pregain.
   ----------------------------------------------------------------*/
EnginePregain::EnginePregain(const char * group)
{
    potmeterPregain = new ControlLogpotmeter(ConfigKey(group, "pregain"), 4.);
    //Replay Gain things
    m_pControlReplayGain = new ControlObject(ConfigKey(group, "replaygain"));
    m_pTotalGain = new ControlObject(ConfigKey(group, "total_gain"));
    m_pPassthroughEnabled = ControlObject::getControl(ConfigKey(group, "passthrough_enabled"));

    if (s_pReplayGainBoost == NULL) {
        s_pReplayGainBoost = new ControlPotmeter(ConfigKey("[ReplayGain]", "InitialReplayGainBoost"),0., 15.);
        s_pEnableReplayGain = new ControlObject(ConfigKey("[ReplayGain]", "ReplayGainEnabled"));
    }

    m_bSmoothFade = false;
    m_fCurrentReplayGain = 0.0f;
    m_fadeOffset = 0.0f;
}

EnginePregain::~EnginePregain()
{
    delete potmeterPregain;
    delete m_pControlReplayGain;
    delete m_pTotalGain;

    delete s_pEnableReplayGain;
    s_pEnableReplayGain = NULL;
    delete s_pReplayGainBoost;
    s_pReplayGainBoost = NULL;
}

void EnginePregain::process(const CSAMPLE* pIn, const CSAMPLE* pOut, const int iBufferSize) {
    float fEnableReplayGain = s_pEnableReplayGain->get();
    float fReplayGainBoost = s_pReplayGainBoost->get();
    CSAMPLE* pOutput = (CSAMPLE*)pOut;
    float fGain = potmeterPregain->get();
    float fReplayGain = m_pControlReplayGain->get();
    float fPassing = m_pPassthroughEnabled->get();

    // TODO(XXX) Why do we do this? Removing it results in clipping at unity
    // gain so I think it was trying to compensate for some issue when we added
    // replaygain but even at unity gain (no RG) we are clipping. rryan 5/2012
    fGain = fGain / 2;

    // Override replaygain value if passing through
    if (fPassing == 1.0) {
        fReplayGain = 1.0;
    } else if ((fReplayGain * fEnableReplayGain) != 0.0) {
        float replayGainCorrection = fReplayGain * pow(10, fReplayGainBoost / 20);
        if (m_fReplayGainCorrection != replayGainCorrection) {
            // need to fade to new replay gain
            // Do not fade when its the initial setup from m_fCurrentReplayGain = 0.0f
            if (m_fadeOffset == 0) {
                m_fadeOffset = replayGainCorrection - m_fReplayGainCorrection;
                m_fadeStart = QTime::currentTime();
            }

            // fade duration 3 second
            float fade_multi = ((float)m_fadeStart.msecsTo(QTime::currentTime())) / 3000;
            if (fade_multi < 1.0f && m_fCurrentReplayGain > 0.0f) {
                //Fade smoothly
                m_fReplayGainCorrection = replayGainCorrection
                        - (m_fadeOffset * (1.0f - fade_multi));
            } else {
                m_fReplayGainCorrection = replayGainCorrection;
                m_fadeOffset = 0.0f;
            }
        }
    } else {
        m_fReplayGainCorrection = 1;
    }
    m_fCurrentReplayGain = fReplayGain;

    // Clamp gain to within [0, 10.0] to prevent insane gains. This can happen
    // (some corrupt files get really high replay gain values).
    // 10 allows a maximum replay Gain Boost * calculated replay gain of ~2
    fGain = fGain * math_max(0.0, math_min(10.0, m_fReplayGainCorrection));

    m_pTotalGain->set(fGain);

    //qDebug()<<"Clock"<<(float)clock()/CLOCKS_PER_SEC;
    // SampleUtil deals with aliased buffers and gains of 1 or 0.
    SampleUtil::copyWithGain(pOutput, pIn, fGain, iBufferSize);
}
