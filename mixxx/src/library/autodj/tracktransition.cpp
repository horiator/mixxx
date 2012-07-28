//tracktransition.cpp

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/autodj/tracktransition.h"
#include "playerinfo.h"
#include "library/dao/cue.h"

TrackTransition::TrackTransition(QObject* parent, ConfigObject<ConfigValue>* pConfig) :
    QObject(parent),
    m_pConfig(pConfig) {
	m_bShortCue = false;
	m_bFadeNow = false;
    // TODO(tom__m) This needs to be updated when the preferences are changed.
    //m_iFadeLength = m_pConfig->getValueString(
                            //ConfigKey("[Auto DJ]", "CrossfadeLength")).toInt();

    m_pCOCrossfader = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Master]", "crossfader")));
    m_pCOPlayPos1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "playposition")));
    m_pCOPlayPos2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "playposition")));
    m_pCOTrackSamples1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "track_samples")));
    m_pCOTrackSamples2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "track_samples")));
    m_pCOPlay1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]","play")));
    m_pCOPlay2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]","play")));
    m_pCOSync1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "beatsync")));
    m_pCOSync2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "beatsync")));
    m_pCOCueOut1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "autodj_cue_out_position")));
    m_pCOCueOut2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "autodj_cue_out_position")));
    m_pCOFadeNowLeft = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[AutoDJ]", "fade_now_left")));
    m_pCOFadeNowRight = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[AutoDJ]", "fade_now_right")));

    connect(m_pCOCrossfader, SIGNAL(valueChanged(double)),
    		this, SLOT(crossfaderChange(double)));
}

TrackTransition::~TrackTransition() {
    delete m_pCOCrossfader;
    delete m_pCOPlayPos1;
    delete m_pCOPlayPos2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
}

void TrackTransition::crossfaderChange(double value) {
	if (!transitioning()) {
		m_dCrossfaderStart = value;
	} else if (value != m_dcrossfadePosition) {
		m_bUserTakeOver = true;
	}
}

void TrackTransition::setGroups(QString groupA, QString groupB) {
	m_groupA = groupA;
	m_groupB = groupB;
	m_trackA = PlayerInfo::Instance().getTrackInfo(m_groupA);
	m_trackB = PlayerInfo::Instance().getTrackInfo(m_groupB);
	// Find cue out for track A
	calculateCue();
	/*if (m_groupA == "[Channel1]") {
	    connect(m_pCOPlayPos1, SIGNAL(valueChanged(double)),
    		this, SLOT(currentPlayPos(double)));
	    disconnect(m_pCOPlayPos2, SIGNAL(valueChanged(double)),
	    		this, SLOT(currentPlayPos(double)));
	} else if (m_groupA == "[Channel2]") {
	    connect(m_pCOPlayPos2, SIGNAL(valueChanged(double)),
	    	this, SLOT(currentPlayPos(double)));
	    disconnect(m_pCOPlayPos1, SIGNAL(valueChanged(double)),
    		this, SLOT(currentPlayPos(double)));
	}*/
}

void TrackTransition::calculateCue() {
	if (!m_trackA) return;
	// Setting m_iEndPoint
	int trackduration = m_trackA->getDuration();
	m_iFadeLength = m_pConfig->getValueString(
			ConfigKey("[Auto DJ]", "Transition")).toInt() *
			m_trackA->getSampleRate() * 2;
	if (m_groupA == "[Channel1]") {
		m_iEndPoint = m_pCOTrackSamples1->get() - m_iFadeLength;
	} else {
		m_iEndPoint = m_pCOTrackSamples2->get() - m_iFadeLength;
	}

	// Setting m_iCuePoint
	int cueOutPoint;
	int pos;
	int samples;
    if (m_groupA == "[Channel1]") {
       	cueOutPoint = m_pCOCueOut1->get();
		samples = m_pCOTrackSamples1->get();
		pos = m_pCOPlayPos1->get() * samples;
    } else if (m_groupA == "[Channel2]") {
       	cueOutPoint = m_pCOCueOut2->get();
		samples = m_pCOTrackSamples2->get();
		pos = m_pCOPlayPos2->get() * samples;
    }
	bool laterThanLoad = true;
	if (cueOutPoint > -1) {
		m_iCuePoint = cueOutPoint;
	} else {
		m_iCuePoint = m_iEndPoint;
	}
	if (m_bFadeNow) {
		qDebug() << "so close...";
		m_iCuePoint = pos;
		m_bFadeNow = false;
	}
	if (pos > m_iEndPoint) {
		qDebug() << "calculating short cue end point";
		calculateShortCue();
		m_iEndPoint = m_iShortCue;
	} else if (pos > m_iCuePoint &&
				pos < m_iCuePoint + m_iFadeLength) {
		qDebug() << "calculating short cue cue point";
		calculateShortCue();
	}
	qDebug() << "Cue Point A analyzed";
}

void TrackTransition::calculateShortCue() {
	if (m_groupA == "[Channel1]") {
		m_iShortCue = m_pCOPlayPos1->get() * m_pCOTrackSamples1->get();
	} else { // m_groupA == "[Channel2]"
		m_iShortCue = m_pCOPlayPos2->get() * m_pCOTrackSamples2->get();
	}
	m_bShortCue = true;
}

// TODO(smstewart): add a boolean so this is only calcualted once
bool TrackTransition::transitioning() {
    int trackSamples;
    double playPos;
	if (m_groupA == "[Channel1]") {
    	trackSamples = m_pCOTrackSamples1->get();
    	playPos = m_pCOPlayPos1->get();
	} else if (m_groupA == "[Channel2]") {
	    trackSamples = m_pCOTrackSamples2->get();
	    playPos = m_pCOPlayPos2->get();
	} else {
		return false;
	}
	m_iCurrentPos = playPos * trackSamples;
    bool afterCue = m_iCurrentPos >= m_iCuePoint;
    bool beforeFadeEnd = m_iCurrentPos < m_iCuePoint + m_iFadeLength * 1.1;
    bool afterEndPoint = m_iCurrentPos >= m_iEndPoint;
    if (!afterEndPoint && !(afterCue && beforeFadeEnd)) {
    	return false;
   	}
    if (afterEndPoint) {
    	m_iFadeStart = m_iEndPoint;
    } else {
    	m_iFadeStart = m_iCuePoint;
    }
	m_iFadeEnd = m_iFadeStart + m_iFadeLength;
	if (m_iFadeEnd > trackSamples) {
	   	m_iFadeEnd = trackSamples;
	}
	if (m_bShortCue) {
		qDebug() << "m_bshortcue returned true";
	   	m_iFadeStart = m_iShortCue;
	}
	return true;
}

void TrackTransition::cueTransition(double value) {
    // Crossfading from Player 1 to Player 2
    if (m_groupA == "[Channel1]" && transitioning()) {
        double crossfadePos = m_dCrossfaderStart + (1 - m_dCrossfaderStart) *
        		((1.0f * m_iCurrentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        //qDebug() << "crossfade position = " << crossfadePos;
        //if (crossfadePos > 1.1) {
        	// Passed by the cue out - wait until end of song to fade
        	//return;
        //}
        if (m_pCOPlay2->get() != 1.0) {
        	m_pCOPlay2->slotSet(1.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos >= 1.0) {
    		m_pCOCrossfader->slotSet(1.0);
    		m_dCrossfaderStart = 1.0;
    		if (m_pCOFadeNowRight->get() == 0.0) {
    		    m_pCOPlay1->slotSet(0.0);
    		} else {
    			m_pCOFadeNowRight->slotSet(0.0);
    		}
    		m_bShortCue = false;
    		emit(transitionDone());
    		qDebug() << "Transition now done";
    	}
    }
    // Crossfading from Player2 to Player 1
    if (m_groupA == "[Channel2]" && transitioning()) {
        double crossfadePos = m_dCrossfaderStart + (-1 - m_dCrossfaderStart) *
        		((1.0f * m_iCurrentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        //qDebug() << "crossfadePos = " << crossfadePos;
        //if (crossfadePos < -1.1) {
        	// Passed by the cue out - wait until end of song to fade
        	//return;
        //}
        if (m_pCOPlay1->get() != 1.0) {
        	m_pCOPlay1->slotSet(1.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos <= -1.0) {
    		m_pCOCrossfader->slotSet(-1.0);
    		m_dCrossfaderStart = -1.0;
    		if (m_pCOFadeNowLeft->get() == 0) {
    		    m_pCOPlay2->slotSet(0.0);
    		} else {
    			m_pCOFadeNowLeft->slotSet(0.0);
    		}
    		emit(transitionDone());
    		m_bShortCue = false;
    		qDebug() << "Transition now done";
    	}
    }
}

void TrackTransition::beatTransition(double value) {
    if (m_groupA == "[Channel1]" && transitioning()) {
        double crossfadePos = m_dCrossfaderStart + (1 - m_dCrossfaderStart) *
        		((1.0f * m_iCurrentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        if (m_pCOPlay2->get() != 1.0) {
        	m_pCOPlay2->slotSet(1.0);
        	// Try only setting to 0
        	m_pCOSync2->slotSet(1.0);
        	m_pCOSync2->slotSet(0.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos >= 1.0) {
    		m_pCOCrossfader->slotSet(1.0);
    		m_dCrossfaderStart = 1.0;
    		if (m_pCOFadeNowRight->get() == 0) {
    		    m_pCOPlay1->slotSet(0.0);
    		} else {
    			m_pCOFadeNowRight->slotSet(0.0);
    		}
    		emit(transitionDone());
    		m_bShortCue = false;
    		qDebug() << "Transition now done";
    	}
    } else if (m_groupA == "[Channel2]" && transitioning()) {
        double crossfadePos = m_dCrossfaderStart + (-1 - m_dCrossfaderStart) *
        		((1.0f * m_iCurrentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        if (m_pCOPlay1->get() != 1.0) {
        	m_pCOPlay1->slotSet(1.0);
        	m_pCOSync1->slotSet(1.0);
        	m_pCOSync1->slotSet(0.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos <= -1.0) {
    		m_pCOCrossfader->slotSet(-1.0);
    		m_dCrossfaderStart = -1.0;
    		if (m_pCOFadeNowLeft->get() == 0) {
    		    m_pCOPlay2->slotSet(0.0);
    		} else {
    			m_pCOFadeNowLeft->slotSet(0.0);
    		}
    		emit(transitionDone());
    		m_bShortCue = false;
    		qDebug() << "Transition now done";
    	}
    }
}

void TrackTransition::cdTransition(double value) {
	if (value == 1.0 || m_pCOFadeNowLeft->get() == 1 ||
			m_pCOFadeNowRight->get() == 1) {
		if (m_groupA == "[Channel1]") {
			//m_pCOPlayPos2->slotSet(0.0);
			m_pCOPlay2->slotSet(1.0);
			m_pCOCrossfader->slotSet(1.0);
			if (m_pCOFadeNowRight->get() == 0) {
			    m_pCOPlay1->slotSet(0.0);
			} else {
				m_pCOFadeNowRight->slotSet(0.0);
			}
		} else if (m_groupA == "[Channel2]") {
			//m_pCOPlayPos1->slotSet(0.0);
			m_pCOPlay1->slotSet(1.0);
			m_pCOCrossfader->slotSet(-1.0);
			if (m_pCOFadeNowLeft->get() == 0) {
			    m_pCOPlay2->slotSet(0.0);
			} else {
				m_pCOFadeNowLeft->slotSet(0.0);
			}
		}
		emit(transitionDone());
	}
}

void TrackTransition::fadeNowLeft() {
	qDebug() << "we've gotten this far";
	m_bFadeNow = true;
    setGroups("[Channel2]", "[Channel1]");
}

void TrackTransition::fadeNowRight() {
    m_bFadeNow = true;
    setGroups("[Channel1]", "[Channel2]");
}

void TrackTransition::fadeNowStopped() {
	m_dCrossfaderStart = m_pCOCrossfader->get();
}
