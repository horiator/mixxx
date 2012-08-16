//autodj.cpp

#include <QtDebug>

#include "configobject.h"
#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "playerinfo.h"
#include "library/autodj/autodj.h"
#include "library/autodj/tracktransition.h"
#include "ui_dlgautodj.h"

AutoDJ::AutoDJ(QObject* parent, ConfigObject<ConfigValue>* pConfig,
		TrackCollection* pTrackCollection) :
    QObject(parent),
    m_pConfig(pConfig),
    m_pTrackCollection(pTrackCollection),
    m_playlistDao(pTrackCollection->getPlaylistDAO()){

    m_btransitionDone = false;
    // Should eventually be changed to m_pconfig value initialization
    m_eTransition = CUE;
    m_eState = ADJ_DISABLED;

    // Most of these COs won't be needed once TrackTransition exists
    m_pCOPlay1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]","play")));
    m_pCOPlay2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]","play")));
    m_pCOPlay1Fb = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "play")));
    m_pCOPlay2Fb = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "play")));
    m_pCORepeat1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "repeat")));
    m_pCORepeat2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "repeat")));
    m_pCOPlayPos1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "playposition")));
    m_pCOPlayPos2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "playposition")));
    m_pCOSampleRate1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "track_samplerate")));
    m_pCOSampleRate2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "track_samplerate")));
    m_pCOTrackSamples1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "track_samples")));
    m_pCOTrackSamples2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "track_samples")));
    m_pCOCrossfader = new ControlObjectThreadMain(
    		ControlObject::getControl(ConfigKey("[Master]", "crossfader")));
    m_pCOScratch1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "scratch2")));
    m_pCOScratch2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "scratch2")));
    m_pCOScratchEnable1 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel1]", "scratch2_enable")));
    m_pCOScratchEnable2 = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Channel2]", "scratch2_enable")));

        // Setting up ControlPushButtons
    m_pCOSkipNext = new ControlPushButton(
                    ConfigKey("[AutoDJ]", "skip_next"));
    connect(m_pCOSkipNext, SIGNAL(valueChanged(double)),
        this, SLOT(skipNext(double)));

    m_pCOFadeNowRight = new ControlPushButton(
                        ConfigKey("[AutoDJ]", "fade_now_right"));
    m_pCOFadeNowRight->setButtonMode(ControlPushButton::TOGGLE);
    m_pCOFadeNowRightThread = new ControlObjectThreadMain(m_pCOFadeNowRight);
    connect(m_pCOFadeNowRightThread, SIGNAL(valueChanged(double)),
        this, SLOT(fadeNowRight(double)));
    m_pCOFadeNowRightThread->slotSet(0.0);

    m_pCOFadeNowLeft = new ControlPushButton(
                       ConfigKey("[AutoDJ]", "fade_now_left"));
    m_pCOFadeNowLeft->setButtonMode(ControlPushButton::TOGGLE);
    m_pCOFadeNowLeftThread = new ControlObjectThreadMain(m_pCOFadeNowLeft);
    connect(m_pCOFadeNowLeftThread, SIGNAL(valueChanged(double)),
        this, SLOT(fadeNowLeft(double)));
    m_pCOFadeNowLeftThread->slotSet(0.0);

    m_pCOShufflePlaylist = new ControlPushButton(
                           ConfigKey("[AutoDJ]", "shuffle_playlist"));
    connect(m_pCOShufflePlaylist, SIGNAL(valueChanged(double)),
        this, SLOT(shufflePlaylist(double)));

    m_pCOToggleAutoDJ = new ControlPushButton(
                        ConfigKey("[AutoDJ]", "toggle_autodj"));
    m_pCOToggleAutoDJ->setButtonMode(ControlPushButton::TOGGLE);
    connect(m_pCOToggleAutoDJ, SIGNAL(valueChanged(double)),
        this, SLOT(toggleAutoDJ(double)));
    m_pCOToggleAutoDJThread = new ControlObjectThreadMain(m_pCOToggleAutoDJ);

    m_pCOSetCueOut1 = new ControlPushButton(
    		      ConfigKey("[AutoDJ]", "set_cue_out_1"));
    m_pCOSetCueOut1->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOSetCueOut1, SIGNAL(valueChanged(double)),
    	this, SLOT(setCueOut1(double)));

    m_pCOSetCueOut2 = new ControlPushButton(
    		      ConfigKey("[AutoDJ]", "set_cue_out_2"));
    m_pCOSetCueOut2->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOSetCueOut2, SIGNAL(valueChanged(double)),
    	this, SLOT(setCueOut2(double)));

    m_pCODeleteCueOut1 = new ControlPushButton(
    		      ConfigKey("[AutoDJ]", "delete_cue_out_1"));
    m_pCODeleteCueOut1->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCODeleteCueOut1, SIGNAL(valueChanged(double)),
    	this, SLOT(deleteCueOut1(double)));

    m_pCODeleteCueOut2 = new ControlPushButton(
    		      ConfigKey("[AutoDJ]", "delete_cue_out_2"));
    m_pCODeleteCueOut2->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCODeleteCueOut2, SIGNAL(valueChanged(double)),
    	this, SLOT(deleteCueOut2(double)));

    // Setting up Playlist
    m_pAutoDJTableModel = new PlaylistTableModel(this, pTrackCollection,
                                                     "mixxx.db.model.autodj");
    int playlistId = m_playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);
    if (playlistId < 0) {
        playlistId = m_playlistDao.createPlaylist(AUTODJ_TABLE,
                                                  PlaylistDAO::PLHT_AUTO_DJ);
    }
    m_pAutoDJTableModel->setPlaylist(playlistId);

    transitionValue = m_pConfig->getValueString(
            ConfigKey("[Auto DJ]", "Transition")).toInt();

    // Setting up ControlObjects for the GUI
    m_pCOCueOutPosition1 = ControlObject::getControl(
    		ConfigKey("[Channel1]", "autodj_cue_out_position"));

    m_pCOCueOutPosition2 = ControlObject::getControl(
    		ConfigKey("[Channel2]", "autodj_cue_out_position"));

    connect(m_pCOPlayPos1, SIGNAL(valueChanged(double)),
            this, SLOT(player1PositionChanged(double)));
    connect(m_pCOPlayPos2, SIGNAL(valueChanged(double)),
            this, SLOT(player2PositionChanged(double)));

    m_pTrackTransition = new TrackTransition(this, m_pConfig);
    connect(m_pTrackTransition, SIGNAL(transitionDone()),
    		this, SLOT(setTransitionDone()));
}

AutoDJ::~AutoDJ() {
	qDebug() << "~AutoDJ()";
    delete m_pCOPlayPos1;
    delete m_pCOPlayPos2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
    delete m_pCOPlay1Fb;
    delete m_pCOPlay2Fb;
    delete m_pCORepeat1;
    delete m_pCORepeat2;
    delete m_pCOSampleRate1;
    delete m_pCOSampleRate2;
    delete m_pCOSkipNext;
    delete m_pCOFadeNowRight;
    delete m_pCOFadeNowLeft;
    delete m_pCOShufflePlaylist;
    delete m_pCOToggleAutoDJ;
    delete m_pCOSetCueOut1;
    delete m_pCOSetCueOut2;
    delete m_pCODeleteCueOut1;
    delete m_pCODeleteCueOut2;
}

PlaylistTableModel* AutoDJ::getTableModel() {
	return m_pAutoDJTableModel;
}

void AutoDJ::player1PositionChanged(double value) {
    if (m_eState == ADJ_DISABLED) {
        //nothing to do
        return;
    }
    if (m_eState == ADJ_FADENOW) {
    	switch (m_eTransition) {
    	   	case CD:
    	   		m_pTrackTransition->cdTransition(value);
    	   		break;
    	   	case CUE:
    	   		m_pTrackTransition->cueTransition(value);
    	   		break;
    	   	case BEAT:
    	   		m_pTrackTransition->beatTransition(value);
    	   		break;
    	}
    	return;
    }
    if (m_eState == ADJ_WAITING && value == 1.0) {
    	// Waiting for a deck to stop and deck 1 is finished
    	m_pCOToggleAutoDJThread->slotSet(1.0);
    }
    bool deck1Playing = m_pCOPlay1Fb->get() == 1.0f;
    bool deck2Playing = m_pCOPlay2Fb->get() == 1.0f;

    if (m_eState == ADJ_ENABLE_P1LOADED) {
        // Auto DJ Start
        if (!deck1Playing && !deck2Playing) {
            m_pCOCrossfader->slotSet(-1.0f);  // Move crossfader to the left!
            // Play the track in player 1
            m_pCOPlay1->slotSet(1.0f);
            removePlayingTrackFromQueue("[Channel1]");
            m_pTrackTransition->setGroups("[Channel1]", "[Channel2]");
        } else {
            m_eState = ADJ_IDLE;
            if (deck1Playing && !deck2Playing) {
                // Here we are, if first deck was playing before starting Auto DJ
                // or if it was started just before
                loadNextTrackFromQueue();
                // if we start the deck from code we don`t get a signal
                player1PlayChanged(1.0f);
                // call function manually
            } else {
                player2PlayChanged(1.0f);
            }
        }
        return;
    }

    if (m_btransitionDone && m_pCOToggleAutoDJ->get() == 1) {
        if (deck1Playing && !deck2Playing) {
            m_eState = ADJ_IDLE;
            m_btransitionDone = false;
            removePlayingTrackFromQueue("[Channel1]");
            loadNextTrackFromQueue();
            m_pTrackTransition->setGroups("[Channel1]", "[Channel2]");
        }
        return;
    }

    if (m_eState == ADJ_IDLE) {
        if (m_pCORepeat1->get() == 1.0f) {
            // repeat disables auto DJ
            return;
        }
    }
    // Fading
    switch (m_eTransition) {
    	case CUE:
    		m_pTrackTransition->cueTransition(value);
    		break;
        case BEAT:
            m_pTrackTransition->beatTransition(value);
            break;
    	case CD:
    		m_pTrackTransition->cdTransition(value);
    		break;
    }
}

void AutoDJ::player2PositionChanged(double value) {
    if (m_eState == ADJ_DISABLED) {
        //nothing to do
        return;
    }
    if (m_eState == ADJ_FADENOW) {
    	switch (m_eTransition) {
    	    case CUE:
    	    	m_pTrackTransition->cueTransition(value);
    	    	break;
    	    case BEAT:
    	        m_pTrackTransition->beatTransition(value);
    	        break;
    	    case CD:
    	    	m_pTrackTransition->cdTransition(value);
    	    	break;
    	}
    	return;
    }
    if (m_eState == ADJ_WAITING && value == 1.0) {
    	// Waiting for a deck to stop and deck 2 is finished
    	qDebug() << "Should be turning AutoDJ on";
    	m_pCOToggleAutoDJThread->slotSet(1.0);
    	qDebug() << "CO = " << m_pCOToggleAutoDJ->get();
    }

    bool deck1Playing = m_pCOPlay1Fb->get() == 1.0f;
    bool deck2Playing = m_pCOPlay2Fb->get() == 1.0f;

    if (m_btransitionDone && m_pCOToggleAutoDJ->get() == 1) {
        if (!deck1Playing && deck2Playing) {
            m_eState = ADJ_IDLE;
            m_btransitionDone = false;
            removePlayingTrackFromQueue("[Channel2]");
            loadNextTrackFromQueue();
            m_pTrackTransition->setGroups("[Channel2]", "[Channel1]");
        }
        return;
    }

    if (m_eState == ADJ_IDLE) {
        if (m_pCORepeat2->get() == 1.0f) {
            // repeat disables auto DJ
            return;
        }
    }
    // Fading
    switch(m_eTransition) {
    	case CUE:
    		m_pTrackTransition->cueTransition(value);
    		break;
        case BEAT:
            m_pTrackTransition->beatTransition(value);
            break;
    	case CD:
    		m_pTrackTransition->cdTransition(value);
    		break;
    }
}

/* Check if this is even necessary at all */
void AutoDJ::player1PlayChanged(double value) {
    if (value == 0.0 && m_pCOScratchEnable1->get() == 1.0) {
    	m_pCOScratch1->slotSet(0.0);
    	m_pCOScratchEnable1->slotSet(0.0);
    }
}

void AutoDJ::player2PlayChanged(double value) {
    if (value == 0.0 && m_pCOScratchEnable2->get() == 1.0) {
    	m_pCOScratch2->slotSet(0.0);
    	m_pCOScratchEnable2->slotSet(0.0);
    }
}

void AutoDJ::transitionValueChanged(int value) {
    if (m_eState == ADJ_IDLE) {
        if (m_pCOPlay1Fb->get() == 1.0f) {
            player1PlayChanged(1.0f);
        }
        if (m_pCOPlay2Fb->get() == 1.0f) {
            player2PlayChanged(1.0f);
        }
    }
    m_pConfig->set(ConfigKey("[Auto DJ]", "Transition"),
                   ConfigValue(value));
    transitionValue = value;
    m_pTrackTransition->calculateCue();
}

void AutoDJ::shufflePlaylist(double value) {
	// Guard to prevent shuffling twice when keyboard sends a double signal
	if (value >= 1.0) return;
   	qDebug() << "Shuffling AutoDJ playlist";
	int row = m_eState == ADJ_DISABLED ? 0 : 1;
   	m_pAutoDJTableModel->shuffleTracks(m_pAutoDJTableModel->index(row, 0));
   	qDebug() << "Shuffling done";
}

void AutoDJ::skipNext(double value) {
	// Guard to prevent shuffling twice when keyboard sends a double signal
	if (value >= 1.0 || m_pCOToggleAutoDJ->get() == 0.0) return;
    qDebug() << "Skip Next";
    // Load the next song from the queue.
    if (m_pCOPlay1Fb->get() == 0.0f) {
        removePlayingTrackFromQueue("[Channel1]");
        loadNextTrackFromQueue();
    } else if (m_pCOPlay2Fb->get() == 0.0f) {
        removePlayingTrackFromQueue("[Channel2]");
        loadNextTrackFromQueue();
    }
    m_pTrackTransition->m_bTrackLoaded = false;
    /* Need a calculate cue call here? */
}

void AutoDJ::fadeNowRight(double value) {
	if (value == 0) {
		if (m_pCOFadeNowLeft->get() == 1) {
			return;
		}
		if (m_eState == ADJ_FADENOW) {
			m_eState = ADJ_DISABLED;
			m_pTrackTransition->fadeNowStopped();
			return;
		} else {
			return;
		}
	}
	if (m_pCOFadeNowLeft->get() == 1) {
		m_pCOFadeNowRightThread->slotSet(0.0);
		return;
	}
	if (m_pCOPlay1->get() == 1) {
        m_pTrackTransition->fadeNowRight();
        if (m_pCOToggleAutoDJ->get() == 0) {
        	m_eState = ADJ_FADENOW;
        } else {
        	m_pCOFadeNowRightThread->slotSet(0.0);
        }
	} else {
		m_pCOFadeNowRightThread->slotSet(0.0);
	}
}

void AutoDJ::fadeNowLeft(double value) {
	if (value == 0) {
		if (m_pCOFadeNowRight->get() == 1) {
			return;
		}
		if (m_eState == ADJ_FADENOW) {
			m_eState = ADJ_DISABLED;
			m_pTrackTransition->fadeNowStopped();
			return;
		} else {
			return;
		}
	}
	if (m_pCOFadeNowRight->get() == 1) {
		m_pCOFadeNowLeftThread->slotSet(0.0);
		return;
	}
	if (m_pCOPlay2->get() == 1) {
        m_pTrackTransition->fadeNowLeft();
        if (m_pCOToggleAutoDJ->get() == 0) {
        	m_eState = ADJ_FADENOW;
        } else {
        	m_pCOFadeNowLeftThread->slotSet(0.0);
        }
	} else {
		m_pCOFadeNowLeftThread->slotSet(0.0);
	}
}

void AutoDJ::toggleAutoDJ(double value) {
	// Guard against double signal sent by keyboard
	if (value == lastToggleValue){
		lastToggleValue = -1.0;
		return;
	}
	lastToggleValue = value;

    bool deck1Playing = m_pCOPlay1Fb->get() == 1.0f;
    bool deck2Playing = m_pCOPlay2Fb->get() == 1.0f;

    if (value) {  // Enable Auto DJ
        qDebug() << "Auto DJ enabled";
        m_pDlgAutoDJ->setAutoDJEnabled();
        m_iCueRecall = m_pConfig->getValueString(
        		ConfigKey("[Controls]" ,"CueRecall")).toInt();
        if (m_eTransition == CD) {
        	m_pConfig->set(ConfigKey("[Controls]", "CueRecall"),
        	               ConfigValue(1));
        }
        if (deck1Playing && deck2Playing) {
            qDebug() << "AutoDJ waiting for one deck to stop";
            lastToggleValue = -1.0;
            m_eState = ADJ_WAITING;
            return;
        }
        // Never load the same track if it is already playing
        if (deck1Playing) {
            removePlayingTrackFromQueue("[Channel1]");
        }
        if (deck2Playing) {
            removePlayingTrackFromQueue("[Channel2]");
        }

        TrackPointer nextTrack = getNextTrackFromQueue();
        if (!nextTrack) {
            qDebug() << "Queue is empty now";
            m_pCOToggleAutoDJThread->slotSet(0.0);
            lastToggleValue = -1.0;
            return;
        }

        // Track is available so GO
         connect(m_pCOPlay1Fb, SIGNAL(valueChanged(double)),
                 this, SLOT(player1PlayChanged(double)));
         connect(m_pCOPlay2Fb, SIGNAL(valueChanged(double)),
                 this, SLOT(player2PlayChanged(double)));

         if (!deck1Playing && !deck2Playing) {
            // both decks are stopped
            m_eState = ADJ_ENABLE_P1LOADED;
            // Force Update on load Track
            player1PositionChanged(-0.001f);
         } else {
            m_eState = ADJ_IDLE;
            if (deck1Playing) {
                // deck 1 is already playing
                player1PlayChanged(1.0f);
            	m_pTrackTransition->setGroups("[Channel1]", "[Channel2]");
            } else {
                // deck 2 is already playing
                player2PlayChanged(1.0f);
            	m_pTrackTransition->setGroups("[Channel2]", "[Channel1]");
            }
        }
        // Loads into first deck If stopped else into second else not
        emit(loadTrack(nextTrack));
    } else {  // Disable Auto DJ
        qDebug() << "Auto DJ disabled";
        m_pDlgAutoDJ->setAutoDJDisabled();
        m_eState = ADJ_DISABLED;
        m_pConfig->set(ConfigKey("[Controls]", "CueRecall"),
        		ConfigValue(m_iCueRecall));
        if (m_pCOScratchEnable1->get() == 1.0) {
        	m_pCOScratch1->slotSet(0.0);
        	m_pCOScratchEnable1->slotSet(0.0);
        }
        if (m_pCOScratchEnable2->get() == 1.0) {
        	m_pCOScratch2->slotSet(0.0);
        	m_pCOScratchEnable2->slotSet(0.0);
        }
        //m_pCOPlayPos1->disconnect(this);
        //m_pCOPlayPos2->disconnect(this);
        //m_pCOPlay1->disconnect(this);
        //m_pCOPlay2->disconnect(this);
    }
}

TrackPointer AutoDJ::getNextTrackFromQueue(){
	// Get the track at the top of the playlist...
	TrackPointer nextTrack;

	while (true) {
	    nextTrack = m_pAutoDJTableModel->getTrack(
	        m_pAutoDJTableModel->index(0, 0));

	    if (nextTrack) {
	        if (nextTrack->exists()) {
	            // found a valid Track
	            return nextTrack;
	        } else {
	            // Remove missing song from auto DJ playlist
	            m_pAutoDJTableModel->removeTrack(
	                m_pAutoDJTableModel->index(0, 0));
	        }
	    } else {
	        // we are running out of tracks
	        break;
	    }
	}
	return nextTrack;
}

bool AutoDJ::loadNextTrackFromQueue() {
    TrackPointer nextTrack = getNextTrackFromQueue();

    // We ran out of tracks in the queue...
    if (!nextTrack) {
        // Disable auto DJ and return...
    	m_pCOToggleAutoDJThread->slotSet(0.0);
        return false;
    }
    emit(loadTrack(nextTrack));
    return true;
}

bool AutoDJ::removePlayingTrackFromQueue(QString group) {
    TrackPointer nextTrack, loadedTrack;
	int nextId = 0, loadedId = 0;

	// Get the track at the top of the playlist...
	nextTrack = m_pAutoDJTableModel->getTrack(m_pAutoDJTableModel->index(0, 0));
	if (nextTrack) {
	    nextId = nextTrack->getId();
	}

	// Get loaded track
	loadedTrack = PlayerInfo::Instance().getTrackInfo(group);
	if (loadedTrack) {
	    loadedId = loadedTrack->getId();
	}

	// When enable auto DJ and Topmost Song is already on second deck, nothing to do
	//BaseTrackPlayer::getLoadedTrack()
	//pTrack = PlayerInfo::Instance().getCurrentPlayingTrack();

	if (loadedId != nextId) {
	    // Do not remove when the user has loaded a track manually
	    return false;
	}

	// remove the top track
	m_pAutoDJTableModel->removeTrack(m_pAutoDJTableModel->index(0, 0));

	return true;
}

void AutoDJ::setDlgAutoDJ(DlgAutoDJ* pDlgAutoDJ) {
	m_pDlgAutoDJ = pDlgAutoDJ;

    //Setting Transition Value
    QString str_autoDjTransition = m_pConfig->getValueString(
            ConfigKey("[Auto DJ]", "Transition"));
    if (str_autoDjTransition.isEmpty()) {
    	// Set 10 as the default
        m_pDlgAutoDJ->spinBoxTransition->setValue(10);
    } else {
    	// Set the value that was set by the user
        m_pDlgAutoDJ->spinBoxTransition->setValue(str_autoDjTransition.toInt());
    }
}

void AutoDJ::setTransitionDone() {
	m_btransitionDone = true;
}

void AutoDJ::transitionSelect(int index) {
	//qDebug() << "transitionSelect called";
	switch (index) {
	case 0:
		m_eTransition = CUE;
		qDebug() << "Transition changed to CUE";
		m_pDlgAutoDJ->spinBoxTransition->setEnabled(true);
		m_pConfig->set(ConfigKey("[Controls]", "CueRecall"),
			           ConfigValue(0));
		m_pTrackTransition->calculateCue();
		break;
	case 1:
		m_eTransition = BEAT;
		qDebug() << "Transition changed to BEAT";
		m_pDlgAutoDJ->spinBoxTransition->setEnabled(true);
		m_pConfig->set(ConfigKey("[Controls]", "CueRecall"),
			           ConfigValue(0));
		m_pTrackTransition->calculateCue();
		break;
	case 2:
		m_eTransition = CD;
		qDebug() << "Transition changed to CD";
		m_pDlgAutoDJ->spinBoxTransition->setEnabled(false);
	    m_pConfig->set(ConfigKey("[Controls]", "CueRecall"),
	                   ConfigValue(1));
		break;
	}
}

void AutoDJ::setCueOut(double position, int channel) {
	// Enforcing uniqueness
	qDebug() << "Deleting old AutoDJ cue to enforce uniqueness";
	deleteCueOut(1.0, channel);
	qDebug() << "Setting AutoDJ cue out for channel " << channel;
	Cue* pCue = NULL;
	int pos = 0;
	TrackPointer track;
	ControlObject* pCOcuePos;
	if (channel == 1) {
		track = PlayerInfo::Instance().getTrackInfo("[Channel1]");
		pCOcuePos = m_pCOCueOutPosition1;
		pos = position * m_pCOTrackSamples1->get();
	}
	if (channel == 2) {
		track = PlayerInfo::Instance().getTrackInfo("[Channel2]");
		pCOcuePos = m_pCOCueOutPosition2;
		pos = position * m_pCOTrackSamples2->get();
	}
	if (track) {
		pCue = track->addCue();
		pCue->setType(Cue::CUEOUT);
		if (pos % 2 != 0) pos--;
		pCue->setPosition(pos);
		pCOcuePos->set(pos);
		track->setCuePoint(pos);
	} else {
		qDebug() << "Null trackpointer - no cue added";
	}
	m_pTrackTransition->calculateCue();
}

void AutoDJ::deleteCueOut(double value, int channel) {
	if (value == 0) return;
	qDebug() << "Deleting cue out from channel " << channel;
	TrackPointer track;
	Cue* pCue = NULL;
	QList<Cue*> cuePoints;
	ControlObject* pCOcuePos;
	if (channel == 1) {
		track = PlayerInfo::Instance().getTrackInfo("[Channel1]");
		pCOcuePos = m_pCOCueOutPosition1;
	}
	if (channel == 2) {
		track = PlayerInfo::Instance().getTrackInfo("[Channel2]");
		pCOcuePos = m_pCOCueOutPosition2;
	}
	if (track) {
		cuePoints = track->getCuePoints();
		QListIterator<Cue*> it(cuePoints);
        while (it.hasNext()) {
            Cue* pCurrentCue = it.next();
            if (pCurrentCue->getType() == Cue::CUEOUT) {
                pCue = pCurrentCue;
            }
        }
        if (pCue != NULL) {
        	track->removeCue(pCue);
        	pCOcuePos->set(-1);
        }
	} else {
		qDebug() << "Null trackpointer - no cue deleted";
	}
}

void AutoDJ::setCueOut1(double value) {
	if (value == 0) return;
	double position = m_pCOPlayPos1->get();
	setCueOut(position, 1);
}

void AutoDJ::setCueOut2(double value) {
	if (value == 0) return;
	double position = m_pCOPlayPos2->get();
	setCueOut(position, 2);
}

void AutoDJ::deleteCueOut1(double value) {
	deleteCueOut(value, 1);
}

void AutoDJ::deleteCueOut2(double value) {
	deleteCueOut(value, 2);
}
