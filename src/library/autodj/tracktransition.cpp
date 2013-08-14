//tracktransition.cpp

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/autodj/tracktransition.h"
#include "playerinfo.h"
#include "library/dao/cue.h"

TrackTransition::TrackTransition(QObject* parent, ConfigObject<ConfigValue>* pConfig)
        : QObject(parent),
          m_bTransitioning(false),
          m_bShortCue(false),
          m_bDeckBCue(false),
          m_bFadeNow(false),
          m_bSpinBack(false),
          m_pConfig(pConfig) {

    m_pCOCrossfader = new ControlObjectThreadMain("[Master]", "crossfader");
    connect(m_pCOCrossfader, SIGNAL(valueChanged(double)),
            this, SLOT(crossfaderChange(double)));
    m_dCrossfaderStart = m_pCOCrossfader->get();
    m_pCOPlayPos1 = new ControlObjectThreadMain("[Channel1]", "playposition");
    m_pCOPlayPos2 = new ControlObjectThreadMain("[Channel2]", "playposition");
    m_pCOTrackSamples1 = new ControlObjectThreadMain("[Channel1]", "track_samples");
    m_pCOTrackSamples2 = new ControlObjectThreadMain("[Channel2]", "track_samples");
    m_pCOPlay1 = new ControlObjectThreadMain("[Channel1]","play");
    m_pCOPlay2 = new ControlObjectThreadMain("[Channel2]","play");
    m_pCOCueOut1 = new ControlObjectThreadMain("[Channel1]", "autodj_cue_out_position");
    m_pCOCueOut2 = new ControlObjectThreadMain("[Channel2]", "autodj_cue_out_position");
    m_pCOFadeNowLeft = new ControlObjectThreadMain("[AutoDJ]", "fade_now_left");
    m_pCOFadeNowRight = new ControlObjectThreadMain("[AutoDJ]", "fade_now_right");

    m_pScratch1 = new ControlObjectThreadMain("[Channel1]", "scratch2");
    m_pScratch2 = new ControlObjectThreadMain("[Channel2]", "scratch2");
    m_pScratchEnable1 = new ControlObjectThreadMain("[Channel1]", "scratch2_enable");
    m_pScratchEnable2 = new ControlObjectThreadMain("[Channel2]", "scratch2_enable");
    m_pCOSync1 = new ControlObjectThreadMain("[Channel1]", "beatsync");
    m_pCOSync2 = new ControlObjectThreadMain("[Channel2]", "beatsync");
    m_pCOSyncPhase1 = new ControlObjectThreadMain("[Channel1]", "beatsync_phase");
    m_pCOSyncPhase2 = new ControlObjectThreadMain("[Channel2]", "beatsync_phase");
    m_pCOSyncTempo1 = new ControlObjectThreadMain("[Channel1]", "beatsync_tempo");
    m_pCOSyncTempo2 = new ControlObjectThreadMain("[Channel2]", "beatsync_tempo");
    m_pCORate1 = new ControlObjectThreadMain("[Channel1]", "rate");
    m_pCORate2 = new ControlObjectThreadMain("[Channel2]", "rate");
    m_pCOBpm1 = new ControlObjectThreadMain("[Channel1]", "bpm");
    m_pCOBpm2 = new ControlObjectThreadMain("[Channel2]", "bpm");
    connect(m_pCOBpm1, SIGNAL(valueChanged(double)),
            this, SLOT(slotBpmChanged(double)));
    connect(m_pCOBpm2, SIGNAL(valueChanged(double)),
            this, SLOT(slotBpmChanged(double)));
}

TrackTransition::~TrackTransition() {
    delete m_pCOCrossfader;
    delete m_pCOPlayPos1;
    delete m_pCOPlayPos2;
    delete m_pCOTrackSamples1;
    delete m_pCOTrackSamples2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
    delete m_pCOCueOut1;
    delete m_pCOCueOut2;
    delete m_pCOFadeNowLeft;
    delete m_pCOFadeNowRight;
    delete m_pScratch1;
    delete m_pScratch2;
    delete m_pScratchEnable1;
    delete m_pScratchEnable2;
    delete m_pCOSync1;
    delete m_pCOSync2;
    delete m_pCOSyncPhase1;
    delete m_pCOSyncPhase2;
    delete m_pCOSyncTempo1;
    delete m_pCOSyncTempo2;
    delete m_pCORate1;
    delete m_pCORate2;
    delete m_pCOBpm1;
    delete m_pCOBpm2;
}

void TrackTransition::crossfaderChange(double value) {
    if (!m_bTransitioning) {
        m_dCrossfaderStart = value;
    }
}

void TrackTransition::setGroups(QString groupFrom, QString groupTo) {
    m_groupFrom = groupFrom;
    m_groupTo = groupTo;
    m_trackFrom = PlayerInfo::Instance().getTrackInfo(m_groupFrom);
    m_trackToPointer = NULL;
    m_bTrackLoaded = false;
    m_bTrackToSynced = false;
    // Find cue out for m_trackFrom
    calculateCueOut();
}

void TrackTransition::calculateCueOut() {
    if (!m_trackFrom) {
        return;
    }

    int cueOutPoint = 0;
    int pos = 0;
    int samples;

    if (m_groupFrom == "[Channel1]") {
        cueOutPoint = m_pCOCueOut1->get();
        samples = m_pCOTrackSamples1->get();
        pos = m_pCOPlayPos1->get() * samples;
    } else if (m_groupFrom == "[Channel2]") {
        cueOutPoint = m_pCOCueOut2->get();
        samples = m_pCOTrackSamples2->get();
        pos = m_pCOPlayPos2->get() * samples;
    } else {
        // No other channels supported
        return;
    }

    // Calculate the m_latestCueOutPoint to full fill desired transitionBeats beats
    int transitionBeats = m_pConfig->getValueString(
            ConfigKey("[Auto DJ]", "Transition")).toInt();
    m_iFadeLength = transitionBeats * m_trackFrom->getSampleRate() * 2 * 60 /
            m_trackFrom->getBpm();
    m_latestCueOutPoint = std::max(samples - m_iFadeLength, pos);

    // Setting m_iCuePoint
    if (m_bFadeNow) {
        m_cueOutPoint = pos;
        m_bFadeNow = false;
    } else if (cueOutPoint >= 0) {
        m_cueOutPoint = cueOutPoint;
    } else {
        m_cueOutPoint = m_latestCueOutPoint;
    }

    if (pos == m_latestCueOutPoint) {
        // The desired transition is not possible
        calculateShortCue();
        m_latestCueOutPoint = m_iShortCue;
    } else if (pos > m_cueOutPoint && pos < m_cueOutPoint + m_iFadeLength) {
        // The desired transition end cannot be reached, but it has not passed
        // TODO() is this correct?
        calculateShortCue();
    }
}

void TrackTransition::calculateShortCue() {
    if ((m_groupFrom == "[Channel1]" && !m_bDeckBCue) ||
        (m_groupFrom == "[Channel2]" && m_bDeckBCue)) {
        m_iShortCue = m_pCOPlayPos1->get() * m_pCOTrackSamples1->get();
    } else if ((m_groupFrom == "[Channel2]" && !m_bDeckBCue) ||
               (m_groupFrom == "[Channel1]" && m_bDeckBCue)) {
        m_iShortCue = m_pCOPlayPos2->get() * m_pCOTrackSamples2->get();
    }
    // TODO() why does m_bDeckBCue switch the deck?
    m_bShortCue = true;
}

void TrackTransition::calculateDeckBCue() {
    if (!m_trackTo || !m_trackFrom) {
        return;
    }

    int trackBSamples;
    if (m_groupFrom == "[Channel1]") {
        trackBSamples = m_pCOTrackSamples2->get();
        m_cueOutPoint = m_pCOPlayPos2->get() * trackBSamples;
        m_latestCueOutPoint = trackBSamples;
    } else if (m_groupFrom == "[Channel2]") {
        trackBSamples = m_pCOTrackSamples1->get();
        m_cueOutPoint = m_pCOPlayPos1->get() * trackBSamples;
        m_latestCueOutPoint = trackBSamples;
    }
    m_iFadeLength = m_iFadeLength / m_trackFrom->getSampleRate() * m_trackTo->getSampleRate();
    // Set fade length shorter when using brake effect transition
    m_iFadeLength /= 2.0;
    m_iFadeEnd = m_cueOutPoint + (m_iFadeEnd - m_iFadeStart) / 2;
    m_bDeckBCue = true;
}

void TrackTransition::transitioning() {
    int trackSamples;
    double playPos;
    if ((m_groupFrom == "[Channel1]" && !m_bDeckBCue) ||
		    (m_groupFrom == "[Channel2]" && m_bDeckBCue)) {
        trackSamples = m_pCOTrackSamples1->get();
        playPos = m_pCOPlayPos1->get();
    } else if ((m_groupFrom == "[Channel2]" && !m_bDeckBCue) ||
               (m_groupFrom == "[Channel1]" && m_bDeckBCue)) {
        trackSamples = m_pCOTrackSamples2->get();
        playPos = m_pCOPlayPos2->get();
    } else {
        // Other Decks not supported
        m_bTransitioning = false;
        return;
    }

    m_iCurrentPos = playPos * trackSamples;
    bool afterCue = m_iCurrentPos >= m_cueOutPoint;
    bool beforeFadeEnd = m_iCurrentPos < m_cueOutPoint + m_iFadeLength * 1.1;
    bool afterLatestCue = m_iCurrentPos >= m_latestCueOutPoint;
    if (!afterLatestCue && !(afterCue && beforeFadeEnd)) {
        // Do not fade if one skips between CuePint and LatestCueOut
        m_bTransitioning = false;
        return;
   	}
    if (afterLatestCue) {
        m_iFadeStart = m_latestCueOutPoint;
    } else {
        m_iFadeStart = m_cueOutPoint;
    }

    if (m_iFadeEnd > trackSamples) {
        // fade length to long
        m_iFadeEnd = trackSamples;
    } else {
        m_iFadeEnd = m_iFadeStart + m_iFadeLength;
    }
    if (m_bShortCue && !m_bDeckBCue) {
        // TODO() Why is m_iFadeEnd not recalculated?
        m_iFadeStart = m_iShortCue;
    }
    m_bTransitioning = true;
}

bool TrackTransition::cueTransition(double value) {
    bool transitionDone = false;
    transitioning();
    if (m_bTransitioning) {
        if (m_groupFrom == "[Channel1]") {
            // Crossfading from Player 1 to Player 2
            double crossfadePos = m_dCrossfaderStart + (1 - m_dCrossfaderStart) *
                    ((1.0f * m_iCurrentPos) - m_iFadeStart) /
                    (m_iFadeEnd - m_iFadeStart);
            if (m_pCOPlay2->get() != 1.0) {
                m_pCOPlay2->set(1.0);
            }
            m_pCOCrossfader->set(crossfadePos);
            if (crossfadePos >= 1.0) {
                m_pCOCrossfader->set(1.0);
                m_dCrossfaderStart = 1.0;
                if (m_pCOFadeNowRight->get() == 0.0) {
                    m_pCOPlay1->set(0.0);
                } else {
                    m_pCOFadeNowRight->set(0.0);
                }
                m_bShortCue = false;
                m_bTransitioning = false;
                transitionDone = true;
            }
        } else if (m_groupFrom == "[Channel2]" && m_bTransitioning) {
            // Crossfading from Player2 to Player 1
            double crossfadePos = m_dCrossfaderStart + (-1 - m_dCrossfaderStart) *
                    ((1.0f * m_iCurrentPos) - m_iFadeStart) /
                    (m_iFadeEnd - m_iFadeStart);
            if (m_pCOPlay1->get() != 1.0) {
                m_pCOPlay1->set(1.0);
            }
            m_pCOCrossfader->set(crossfadePos);
            if (crossfadePos <= -1.0) {
                m_pCOCrossfader->set(-1.0);
                m_dCrossfaderStart = -1.0;
                if (m_pCOFadeNowLeft->get() == 0.0) {
                    m_pCOPlay2->set(0.0);
                } else {
                    m_pCOFadeNowLeft->set(0.0);
                }
                transitionDone = true;
                m_bShortCue = false;
                m_bTransitioning = false;
            }
        }
    }
    return transitionDone;
}

bool TrackTransition::beatTransition(double value) {
    bool transitionDone = false;
    if (m_trackToPointer == NULL || !m_bTrackLoaded) {
        return transitionDone;
    }
    if (!m_bTrackToSynced) {
        if (m_dBpmShift > 0.94 && m_dBpmShift < 1.06) {
            if (m_groupFrom == "[Channel1]") {
                m_pCOSyncTempo2->set(1.0);
                m_pCOSyncTempo2->set(0.0);
            } else {
                m_pCOSyncTempo1->set(1.0);
                m_pCOSyncTempo1->set(0.0);
            }
        } else {
            if (m_groupFrom == "[Channel1]") {
                m_pCORate2->set(0.0);
            } else {
                m_pCORate1->set(0.0);
            }
        }
        m_bTrackToSynced = true;
        return transitionDone;
    }
    transitioning();
    double crossfadePos;
    if (m_groupFrom == "[Channel1]" && (m_bTransitioning || m_bSpinBack)) {
        m_dBpmFrom = m_pCOBpm1->get();
        m_dBpmTo = m_pCOBpm2->get();
        m_dBpmShift = m_dBpmFrom / m_dBpmTo;
        if (m_dBpmShift > 0.94 && m_dBpmShift < 1.06) {
            if (m_pCOPlay2->get() != 1.0) {
                m_pCOPlay2->set(1.0);
                m_pCOSync2->set(1.0);
                m_pCOSync2->set(0.0);
            }
            crossfadePos = m_dCrossfaderStart
                    + (1 - m_dCrossfaderStart) * ((1.0f * m_iCurrentPos) - m_iFadeStart)
                    / (m_iFadeEnd - m_iFadeStart);
        } else if (m_dBpmShift >= 1.06) {
            if (!m_bDeckBCue) {
                calculateDeckBCue();
                m_pScratchEnable1->set(1.0);
                transitioning();
            }
            if (m_pCOPlay2->get() != 1.0) {
                m_pCOPlay2->set(1.0);
            }
            crossfadePos = m_dCrossfaderStart
                    + (1 - m_dCrossfaderStart) * ((1.0f * m_iCurrentPos) - m_iFadeStart)
                    / (m_iFadeEnd - m_iFadeStart);
            m_dBrakeRate = 1 - (crossfadePos + 1) / 2.0;
            spinBackTransition(m_dBrakeRate);
            if (crossfadePos >= 1) {
                m_pScratch1->set(0.0);
                m_pScratchEnable1->set(0.0);
                m_bDeckBCue = false;
            }
        } else if (m_dBpmShift <= 0.94) {
            if (!m_bSpinBack) {
                m_pScratchEnable1->set(1.0);
                m_bSpinBack = true;
                m_dSpinRate = -5;
                crossfadePos = 0.0;
                m_pCOPlay2->set(1.0);
            }
            if (m_dSpinRate < 1) {
                m_dSpinRate += 0.2;
                spinBackTransition(m_dSpinRate);
            } else {
                spinBackTransition(0.0);
                m_pScratchEnable1->set(0.0);
                crossfadePos = 1.0;
                m_bSpinBack = false;
            }
        }
        m_pCOCrossfader->set(crossfadePos);
        if (crossfadePos >= 1.0) {
            m_pCOCrossfader->set(1.0);
            m_dCrossfaderStart = 1.0;
            if (m_pCOFadeNowRight->get() == 0) {
                m_pCOPlay1->set(0.0);
            } else {
                m_pCOFadeNowRight->set(0.0);
            }
            transitionDone = true;
            m_bShortCue = false;
            m_bTransitioning = false;
            m_dBrakeRate = 1;
        }
    } else if (m_groupFrom == "[Channel2]" && (m_bTransitioning || m_bSpinBack)) {
        m_dBpmFrom = m_pCOBpm2->get();
        m_dBpmTo = m_pCOBpm1->get();
        m_dBpmShift = m_dBpmFrom / m_dBpmTo;
        if (m_dBpmShift > 0.94 && m_dBpmShift < 1.06) {
            if (m_pCOPlay1->get() != 1.0) {
                m_pCOPlay1->set(1.0);
                m_pCOSync1->set(1.0);
                m_pCOSync1->set(0.0);
            }
            crossfadePos = m_dCrossfaderStart
                    + (-1 - m_dCrossfaderStart)
                    * ((1.0f * m_iCurrentPos) - m_iFadeStart)
                    / (m_iFadeEnd - m_iFadeStart);
        } else if (m_dBpmShift >= 1.06) {
            if (!m_bDeckBCue) {
                calculateDeckBCue();
                m_pScratchEnable2->set(1.0);
                transitioning();
            }
            if (m_pCOPlay1->get() != 1.0) {
                m_pCOPlay1->set(1.0);
            }
            if (m_iCurrentPos == 0 && m_iFadeEnd == 0) {
                return transitionDone;
            }
            crossfadePos = m_dCrossfaderStart
                    + (-1 - m_dCrossfaderStart)
                    * (((1.0f * m_iCurrentPos) - m_iFadeStart)
                            / (m_iFadeEnd - m_iFadeStart));
            m_dBrakeRate = (crossfadePos + 1) / 2.0;
            spinBackTransition(m_dBrakeRate);
            if (crossfadePos <= -1) {
                m_pScratch2->set(0.0);
                m_pScratchEnable2->set(0.0);
                m_bDeckBCue = false;
            }
        } else if (m_dBpmShift <= 0.94) {
            if (!m_bSpinBack) {
                m_pScratchEnable2->set(1.0);
                m_bSpinBack = true;
                m_dSpinRate = -5;
                crossfadePos = 0.0;
                m_pCOPlay1->set(1.0);
            }
            if (m_dSpinRate < 1) {
                m_dSpinRate += 0.2;
                spinBackTransition(m_dSpinRate);
            } else {
                spinBackTransition(0.0);
                m_pScratchEnable2->set(0.0);
                crossfadePos = -1.0;
                m_bSpinBack = false;
            }
        }
        m_pCOCrossfader->set(crossfadePos);
        if (crossfadePos <= -1.0) {
            m_pCOCrossfader->set(-1.0);
            m_dCrossfaderStart = -1.0;
            if (m_pCOFadeNowLeft->get() == 0) {
                m_pCOPlay2->set(0.0);
            } else {
                m_pCOFadeNowLeft->set(0.0);
            }
            transitionDone = true;
            m_dBrakeRate = 1;
            m_bShortCue = false;
            m_bTransitioning = false;
        }
    }
    return transitionDone;
}

bool TrackTransition::cdTransition(double value) {
    bool transitionDone = false;
    transitioning();
    if (value == 1.0 || m_pCOFadeNowLeft->get() == 1 ||
        m_pCOFadeNowRight->get() == 1) {
        if (m_groupFrom == "[Channel1]") {
            m_pCOPlay2->set(1.0);
            m_pCOCrossfader->set(1.0);
            if (m_pCOFadeNowRight->get() == 0) {
                m_pCOPlay1->set(0.0);
            } else {
                m_pCOFadeNowRight->set(0.0);
            }
        } else if (m_groupFrom == "[Channel2]") {
            m_pCOPlay1->set(1.0);
            m_pCOCrossfader->set(-1.0);
            if (m_pCOFadeNowLeft->get() == 0) {
                m_pCOPlay2->set(0.0);
            } else {
                m_pCOFadeNowLeft->set(0.0);
            }
        }
        transitionDone = true;
    }
    return transitionDone;
}

void TrackTransition::spinBackTransition(double value) {
    if (m_groupFrom == "[Channel1]" && (m_bTransitioning || m_bSpinBack)) {
        m_pScratch1->set(value);
    } else if (m_groupFrom == "[Channel2]" && (m_bTransitioning || m_bSpinBack)) {
        m_pScratch2->set(value);
    }
}

void TrackTransition::brakeTransition(double rate) {
    if (m_groupFrom == "[Channel1]" && m_bTransitioning) {
        //m_pCOJog1->slotSet(rate);
    } else if (m_groupFrom == "[Channel2]" && m_bTransitioning) {
        //m_pCOJog2->slotSet(rate);
    }
}

void TrackTransition::fadeNowLeft() {
    m_bFadeNow = true;
    setGroups("[Channel2]", "[Channel1]");
}

void TrackTransition::fadeNowRight() {
    m_bFadeNow = true;
    setGroups("[Channel1]", "[Channel2]");
}

void TrackTransition::fadeNowStopped() {
    m_dCrossfaderStart = m_pCOCrossfader->get();
    m_bTransitioning = false;
}

void TrackTransition::slotBpmChanged(double value) {
    loadTrack();
}

void TrackTransition::loadTrack() {
    if (m_pCOBpm1 == 0 || m_pCOBpm2 == 0 || m_bTrackToSynced) return;
    if (m_trackToPointer == NULL) {
        m_trackTo = PlayerInfo::Instance().getTrackInfo(m_groupTo);
        if (!m_trackTo) {
            return;
        }
        m_trackToPointer = &m_trackTo;
    }
    if (m_groupFrom == "[Channel1]") {
        m_dBpmFrom = m_pCOBpm1->get();
        m_dBpmTo = m_pCOBpm2->get();
    } else {
        m_dBpmFrom = m_pCOBpm2->get();
        m_dBpmTo = m_pCOBpm1->get();
    }
    m_dBpmShift = m_dBpmFrom / m_dBpmTo;
    m_dBrakeRate = 1;
    m_dSpinRate = 0;
    m_bTrackToSynced = false;
    m_bTrackLoaded = true;
}
