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
	Cue* pCueOut = NULL;
	Cue* loadCue = NULL;
	QList<Cue*> cuePoints;
	if (m_trackA) {
		pCueOut = NULL;
		cuePoints = m_trackA->getCuePoints();
		QListIterator<Cue*> it(cuePoints);
	    while (it.hasNext()) {
	        Cue* pCurrentCue = it.next();
	        if (pCurrentCue->getType() == Cue::AUTODJ) {
	            pCueOut = pCurrentCue;
	        } else if(pCurrentCue->getType() == Cue::LOAD) {
	        	loadCue = pCurrentCue;
	        }
	    }
	    bool laterThanLoad = true;
	    if (pCueOut != NULL) {
	    	if (loadCue !=NULL) {
	    		// Cue out has to be later in the song than the loading cue
	    		laterThanLoad = loadCue->getPosition() < pCueOut->getPosition();
	    		qDebug() << "loadcue position = " << loadCue->getPosition() <<
	    				" and cueout position = " << pCueOut->getPosition();
	    	}
	    	if (laterThanLoad) {
	    		m_icuePoint = pCueOut->getPosition();
	    		qDebug() << "Track A cue point = " << m_icuePoint;
	    	}
	    }
	    if (pCueOut == NULL || !laterThanLoad) {
	    	int trackduration = m_trackA->getDuration();
	    	int fadelength = m_pConfig->getValueString(
	        	ConfigKey("[Auto DJ]", "Transition")).toInt();
	    	if (m_groupA == "[Channel1]") {
	    		m_icuePoint = m_pCOTrackSamples1->get() -
	    				(m_trackA->getSampleRate() * fadelength);
	    	} else {
	    		m_icuePoint = m_pCOTrackSamples2->get() -
	    				(m_trackA->getSampleRate() * fadelength);
	    	}
	    	qDebug() << "Track A calculated cue point = " << m_icuePoint;
	    }
	    qDebug() << "Cue Point A analyzed";
	}
	/*
	if (m_trackB) {
		pCue = NULL;
		cuePoints = m_trackB->getCuePoints();
		QListIterator<Cue*> it(cuePoints);
	    while (it.hasNext()) {
	        Cue* pCurrentCue = it.next();
	        if (pCurrentCue->getType() == Cue::AUTODJ) {
	            pCue = pCurrentCue;
	        }
	    }
	    if (pCue != NULL) {
	    	m_iTrackBCue = pCue->getPosition();
	    	qDebug() << "Track B cue point = " << m_iTrackBCue;
	    }
	    qDebug() << "Cue Point B analyzed";
	}*/
}

void TrackTransition::cueTransition(double value) {
	int currentPos;
    // Crossfading from Player 1 to Player 2
    if (m_groupA == "[Channel1]" && m_groupB == "[Channel2]") {
    	int trackSamples = m_pCOTrackSamples1->get();
    	currentPos = value * trackSamples;
    	// Return if position is not to the cue point yet
    	if (currentPos < m_icuePoint) return;
    	if (m_pCOPlay2->get() != 1.0) {
    		m_pCOPlay2->slotSet(1.0);
    	}
    	// fade length in samples
    	m_iFadeLength = m_pConfig->getValueString(
    		ConfigKey("[Auto DJ]", "Transition")).toInt() *
    		m_trackA->getSampleRate();
    	int fadeEnd = m_icuePoint + m_iFadeLength;
    	if (fadeEnd > trackSamples) {
    		fadeEnd = trackSamples;
    	}
    	double crossfadePos = -1.0 + 2.0 * ((1.0f * currentPos) - m_icuePoint) /
    				          (fadeEnd - m_icuePoint);
    	qDebug() << "(top) crossfade position = " << crossfadePos;
    	m_pCOCrossfader->slotSet(crossfadePos);
    	if (crossfadePos >= 1.0) {
    		m_pCOCrossfader->slotSet(1.0);
    		m_pCOPlay1->slotSet(0.0);
    		emit(transitionDone());
    	}
    }

    // Crossfading from Player2 to Player 1
    if (m_groupA == "[Channel2]" && m_groupB == "[Channel1]") {
    	int trackSamples = m_pCOTrackSamples2->get();
    	currentPos = value * trackSamples;
    	// Return if position is not to the cue point yet
    	if (currentPos < m_icuePoint) return;
    	if (m_pCOPlay1->get() != 1.0) {
    		m_pCOPlay1->slotSet(1.0);
    	}
    	m_iFadeLength = m_pConfig->getValueString(
    		ConfigKey("[Auto DJ]", "Transition")).toInt() *
    		m_trackA->getSampleRate();
    	int fadeEnd = m_icuePoint + m_iFadeLength;
    	if (fadeEnd > trackSamples) {
    		fadeEnd = trackSamples;
    	}
    	double crossfadePos = 1.0 - 2.0 * ((1.0f * currentPos) - m_icuePoint) /
    				          (fadeEnd - m_icuePoint);
    	qDebug() << "(bottom) crossfade position = " << crossfadePos;
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
		qDebug() << "m_groupA = " << m_groupA << " and m_groupB = " << m_groupB;
		if (m_groupA == "[Channel1]") {
			m_pCOPlayPos2->slotSet(0.0);
			m_pCOPlay2->slotSet(1.0);
			m_pCOCrossfader->slotSet(1.0);
			m_pCOPlay1->slotSet(0.0);
		} else if (m_groupA == "[Channel2]") {
			m_pCOPlayPos1->slotSet(0.0);
			m_pCOPlay1->slotSet(1.0);
			m_pCOCrossfader->slotSet(-1.0);
			m_pCOPlay2->slotSet(0.0);
		}
		emit(transitionDone());
	}
}
