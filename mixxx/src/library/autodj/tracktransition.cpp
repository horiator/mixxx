//tracktransition.cpp

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/autodj/tracktransition.h"
#include "playerinfo.h"
#include "library/dao/cue.h"

TrackTransition::TrackTransition(QObject* parent, ConfigObject<ConfigValue>* pConfig) :
    QObject(parent),
    m_pConfig(pConfig) {

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
}

TrackTransition::~TrackTransition() {
    delete m_pCOCrossfader;
    delete m_pCOPlayPos1;
    delete m_pCOPlayPos2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
}

void TrackTransition::checkForUserInput(double value) {
	if(value != m_dcrossfadePosition) {
		m_buserTakeOver = true;
	}
}

void TrackTransition::setGroups(QString groupA, QString groupB) {
	m_groupA = groupA;
	m_groupB = groupB;
	m_trackA = PlayerInfo::Instance().getTrackInfo(m_groupA);
	m_trackB = PlayerInfo::Instance().getTrackInfo(m_groupB);
	// Find cue out for track A
	if (m_trackA) {
		calculateCue();
	}
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
	if (m_trackA) {
		// Setting m_iEndPoint
		int trackduration = m_trackA->getDuration();
		int fadelength = m_pConfig->getValueString(
				ConfigKey("[Auto DJ]", "Transition")).toInt();
		if (m_groupA == "[Channel1]") {
			m_iEndPoint = m_pCOTrackSamples1->get() -
					(m_trackA->getSampleRate() * fadelength * 2.0);
		} else {
			m_iEndPoint = m_pCOTrackSamples2->get() -
					(m_trackA->getSampleRate() * fadelength * 2.0);
		}

		// Setting m_icuePoint
		Cue* pCueOut = NULL;
		Cue* loadCue = NULL;
		QList<Cue*> cuePoints = m_trackA->getCuePoints();
		QListIterator<Cue*> it(cuePoints);
		while (it.hasNext()) {
			Cue* pCurrentCue = it.next();
			if (pCurrentCue->getType() == Cue::CUEOUT) {
				pCueOut = pCurrentCue;
			} else if(pCurrentCue->getType() == Cue::LOAD) {
				loadCue = pCurrentCue;
			}
		}
		bool laterThanLoad = true;
		if (pCueOut != NULL) {// && !m_bPastCue) {
			if (loadCue !=NULL) {
				// Cue out has to be later in the song than the loading cue
				// and later than the current play position
				laterThanLoad = loadCue->getPosition() < pCueOut->getPosition();
				qDebug() << "loadcue position = " << loadCue->getPosition() <<
    				" and cueout position = " << pCueOut->getPosition();
			}
			if (laterThanLoad) {
				m_icuePoint = pCueOut->getPosition();
				qDebug() << "Track A cue point = " << m_icuePoint;
			}
		}
		if (pCueOut == NULL || !laterThanLoad) {// || m_bPastCue) {
			m_icuePoint = m_iEndPoint;
		}
		if (m_groupA == "[Channel1]" &&
				m_pCOPlayPos1->get() * m_pCOTrackSamples1->get() > m_iEndPoint) {
			calculateShortCue(m_groupA);
			m_iEndPoint = m_iShortCue;
		}
		if (m_groupA == "[Channel2]" &&
				m_pCOPlayPos2->get() * m_pCOTrackSamples2->get() > m_iEndPoint) {
			calculateShortCue(m_groupA);
			m_iEndPoint = m_iShortCue;
		}

		/*int trackduration = m_trackA->getDuration();
			int fadelength = m_pConfig->getValueString(
					ConfigKey("[Auto DJ]", "Transition")).toInt();
			if (m_groupA == "[Channel1]") {
				m_icuePoint = m_pCOTrackSamples1->get() -
						(m_trackA->getSampleRate() * fadelength * 2.0);
			} else {
				m_icuePoint = m_pCOTrackSamples2->get() -
						(m_trackA->getSampleRate() * fadelength * 2.0);
			}
			qDebug() << "Track A calculated cue point = " << m_icuePoint;
		}*/
		qDebug() << "Cue Point A analyzed";
	}
}

void TrackTransition::calculateShortCue(QString channel) {
	if (channel == m_groupA) { // Short cue at end of song
		if (m_groupA == "[Channel1]") {
			m_iShortCue = m_pCOPlayPos1->get() * m_pCOTrackSamples1->get();
		} else { // m_groupA == "[Channel2]"
			m_iShortCue = m_pCOPlayPos2->get() * m_pCOTrackSamples2->get();
		}
	}
	if (channel == m_groupB) {
		if (m_groupA == "[Channel1]") {

		}
	}
	m_bShortCue = true;
}

void TrackTransition::cueTransition(double value) {
	int currentPos;
	int trackSamples;
    // Crossfading from Player 1 to Player 2
    if (m_groupA == "[Channel1]" && m_groupB == "[Channel2]") {
    	trackSamples = m_pCOTrackSamples1->get();
    	currentPos = value * trackSamples;
        m_iFadeLength = m_pConfig->getValueString(
        	ConfigKey("[Auto DJ]", "Transition")).toInt() *
        	m_trackA->getSampleRate();

        bool afterCue = currentPos >= m_icuePoint;
        bool beforeFadeEnd = currentPos < m_icuePoint + m_iFadeLength * 2 * 1.1;
        bool afterEndPoint = currentPos >= m_iEndPoint;
    	if (!afterEndPoint && !(afterCue && beforeFadeEnd)) {
    		return;
    	}
        if (afterCue && beforeFadeEnd) {
    		m_iFadeStart = m_icuePoint;
    	}
    	if (afterEndPoint) {
    		m_iFadeStart = m_iEndPoint;
    	}

        m_iFadeEnd = m_iFadeStart + 2.0 * m_iFadeLength;
        if (m_iFadeEnd > trackSamples) {
        	m_iFadeEnd = trackSamples;
        }
        double crossfadePos = -1.0 + 2.0 * ((1.0f * currentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        qDebug() << "crossfade position = " << crossfadePos;
        if (crossfadePos > 1.1) {
        	// Passed by the cue out - wait until end of song to fade
        	return;
        }
        if (m_pCOPlay2->get() != 1.0) {
        	m_pCOPlay2->slotSet(1.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos >= 1.0) {
    		m_pCOCrossfader->slotSet(1.0);
    		m_pCOPlay1->slotSet(0.0);
    		emit(transitionDone());
    	}
    }
    // Crossfading from Player2 to Player 1
    if (m_groupA == "[Channel2]" && m_groupB == "[Channel1]") {
    	trackSamples = m_pCOTrackSamples2->get();
    	currentPos = value * trackSamples;
        m_iFadeLength = m_pConfig->getValueString(
        	ConfigKey("[Auto DJ]", "Transition")).toInt() *
        	m_trackA->getSampleRate();

    	if (currentPos >= m_icuePoint && currentPos < m_icuePoint + m_iFadeLength * 2 * 1.1) {
    		m_iFadeStart = m_icuePoint;
    	} else if (currentPos >= m_iEndPoint) {
    		m_iFadeStart = m_iEndPoint;
    	} else {
    		return;
    	}

        m_iFadeLength = m_pConfig->getValueString(
        	ConfigKey("[Auto DJ]", "Transition")).toInt() *
        	m_trackA->getSampleRate();
        m_iFadeEnd = m_iFadeStart + 2.0 * m_iFadeLength;
        if (m_iFadeEnd > trackSamples) {
        	m_iFadeEnd = trackSamples;
        }
        double crossfadePos = 1.0 - 2.0 * ((1.0f * currentPos) - m_iFadeStart) /
        		(m_iFadeEnd - m_iFadeStart);
        qDebug() << "crossfadePos = " << crossfadePos;
        if (crossfadePos < -1.1) {
        	// Passed by the cue out - wait until end of song to fade
        	return;
        }
        if (m_pCOPlay1->get() != 1.0) {
        	m_pCOPlay1->slotSet(1.0);
        }
        m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos <= -1.0) {
    		m_pCOCrossfader->slotSet(-1.0);
    		m_pCOPlay2->slotSet(0.0);
    		emit(transitionDone());
    	}
    }
}

void TrackTransition::cdTransition(double value) {
	if (value == 1.0) {
		//qDebug() << "m_groupA = " << m_groupA << " and m_groupB = " << m_groupB;
		if (m_groupA == "[Channel1]") {
			//m_pCOPlayPos2->slotSet(0.0);
			m_pCOPlay2->slotSet(1.0);
			m_pCOCrossfader->slotSet(1.0);
			m_pCOPlay1->slotSet(0.0);
		} else if (m_groupA == "[Channel2]") {
			//m_pCOPlayPos1->slotSet(0.0);
			m_pCOPlay1->slotSet(1.0);
			m_pCOCrossfader->slotSet(-1.0);
			m_pCOPlay2->slotSet(0.0);
		}
		emit(transitionDone());
	}
}
