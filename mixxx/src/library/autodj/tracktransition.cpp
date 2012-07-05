//tracktransition.cpp

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/autodj/tracktransition.h"
#include "playerinfo.h"

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
    //m_pCOTrackSamples1 = new ControlObjectThreadMain(
                            //ControlObject::getControl(ConfigKey("[Channel1]", "track_samples")));
    //m_pCOTrackSamples2 = new ControlObjectThreadMain(
                            //ControlObject::getControl(ConfigKey("[Channel2]", "track_samples")));
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
}

void TrackTransition::cueTransition(double value) {

    // Crossfading from Player 1 to Player 2
    if (m_groupA == "[Channel1]" && m_groupB == "[Channel2]") {
        // Find the FadeOut point position and crossfade length as ratios
        // of the whole track. posFadeEnd is then the position where the crossfade
        // will stop.
        float sampleThreshold = 0.0f; //m_trackA->getFadeOut() / m_pCOTrackSamples1->get();
        float fadeLengthRatio = (float) m_iFadeLength / m_trackA->getDuration();
        float posFadeEnd = sampleThreshold + fadeLengthRatio;

        // Check if our crossfade would stop past the end of the track
        if (posFadeEnd > 1.0) {
            posFadeEnd = 1.0;
        }

        m_dcrossfadePosition = -1.0f + 2*(m_pCOPlayPos1->get()-sampleThreshold) / (posFadeEnd - sampleThreshold);
        m_pCOCrossfader->slotSet(m_dcrossfadePosition);
    }

    // Crossfading from Player2 to Player 1
    if (m_groupA == "[Channel2]" && m_groupB == "[Channel1]") {

        // Find the FadeOut point position and crossfade length as ratios
        // of the whole track. posFadeEnd is then the position where the crossfade
        // will stop.
        float sampleThreshhold = 0.0f; //m_trackA->getFadeOut() / m_pCOTrackSamples2->get();
        float fadeLengthRatio = (float) m_iFadeLength / m_trackA->getDuration();
        float posFadeEnd = sampleThreshhold + fadeLengthRatio;

        // Check if our crossfade would stop past the end of the track
        if (posFadeEnd > 1.0) {
            posFadeEnd = 1.0;
        }

        m_dcrossfadePosition = 1.0f - 2*(m_pCOPlayPos2->get()-sampleThreshhold) / (posFadeEnd - sampleThreshhold);
        m_pCOCrossfader->slotSet(m_dcrossfadePosition);

    }
    if(m_dcrossfadePosition == 1.0 || m_dcrossfadePosition == -1.0) {
    	// Break the while loop and stop transitioning
    	return;
    }
}

void TrackTransition::cdTransition(double value) {
	if (value == 1.0) {
		if (m_groupA == "[Channel1]") {
			m_pCOPlayPos2->slotSet(0.0);
			m_pCOPlay2->slotSet(1.0);
			m_pCOCrossfader->slotSet(1.0);
			m_pCOPlay1->slotSet(0.0);
		} else if (m_groupB == "[Channel2]") {
			m_pCOPlayPos1->slotSet(0.0);
			m_pCOPlay1->slotSet(1.0);
			m_pCOCrossfader->slotSet(-1.0);
			m_pCOPlay2->slotSet(0.0);
		}
	}
}
