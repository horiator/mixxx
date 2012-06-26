//autodj.cpp

#include <QtDebug>

#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "playerinfo.h"
#include "library/autodj.h"
#include "library/tracktransition.h"

AutoDJ::AutoDJ(QObject* parent, ConfigObject<ConfigValue>* pConfig,
		TrackCollection* pTrackCollection) :
    QObject(parent),
    m_bEnabled(false),
    m_pConfig(pConfig),
    m_pTrackCollection(pTrackCollection),
    m_playlistDao(pTrackCollection->getPlaylistDAO()){

    m_bPlayer1Primed = false;
    m_bPlayer2Primed = false;
    m_bPlayer1Cued = false;
    m_bPlayer2Cued = false;
    m_bEndOfPlaylist = false;

    //m_pTrackTransition = new TrackTransition(this, m_pConfig);

    m_pCOPlay1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "play")));
    m_pCOPlay2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "play")));
    m_pCORepeat1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "repeat")));
    m_pCORepeat2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "repeat")));
    m_pCOPlayPosSamples1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "playposition_samples")));
    m_pCOPlayPosSamples2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "playposition_samples")));
    m_pCOSampleRate1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "track_samplerate")));
    m_pCOSampleRate2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "track_samplerate")));

    // Setting up ControlPushButtons
    m_pCOSkipNext = new ControlPushButton(
                    ConfigKey("[AutoDJ]", "skip_next"));
    m_pCOSkipNext->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOSkipNext, SIGNAL(valueChanged(double)),
        this, SLOT(skipNext(double)));

    m_pCOFadeNowRight = new ControlPushButton(
                        ConfigKey("[AutoDJ]", "fade_now_right"));
    m_pCOFadeNowRight->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOFadeNowRight, SIGNAL(valueChanged(double)),
        this, SLOT(fadeNowRight(double)));

    m_pCOFadeNowLeft = new ControlPushButton(
                       ConfigKey("[AutoDJ]", "fade_now_left"));
    m_pCOFadeNowLeft->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOFadeNowLeft, SIGNAL(valueChanged(double)),
        this, SLOT(fadeNow(double)));

    m_pCOShufflePlaylist = new ControlPushButton(
                           ConfigKey("[AutoDJ]", "shuffle_playlist"));
    m_pCOShufflePlaylist->setButtonMode(ControlPushButton::PUSH);
    connect(m_pCOShufflePlaylist, SIGNAL(valueChanged(double)),
        this, SLOT(shufflePlaylist(double)));

    m_pCOToggleAutoDJ = new ControlPushButton(
                        ConfigKey("[AutoDJ]", "toggle_autodj"));
    m_pCOToggleAutoDJ->setButtonMode(ControlPushButton::TOGGLE);
    connect(m_pCOToggleAutoDJ, SIGNAL(valueChanged(double)),
        this, SLOT(toggleAutoDJ(double)));

    // Setting up Playlist
    m_pAutoDJTableModel = new PlaylistTableModel(this, pTrackCollection,
                                                     "mixxx.db.model.autodj");
    int playlistId = m_playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);
    if (playlistId < 0) {
        playlistId = m_playlistDao.createPlaylist(AUTODJ_TABLE,
                                                  PlaylistDAO::PLHT_AUTO_DJ);
    }
    m_pAutoDJTableModel->setPlaylist(playlistId);
}

AutoDJ::~AutoDJ() {
    delete m_pCOPlayPosSamples1;
    delete m_pCOPlayPosSamples2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
    delete m_pCORepeat1;
    delete m_pCORepeat2;
    delete m_pCOSampleRate1;
    delete m_pCOSampleRate2;
}

PlaylistTableModel* AutoDJ::getTableModel() {
	return m_pAutoDJTableModel;
}

void AutoDJ::setEnabled(bool enable) {

    // Check if we have yet to receive a track to load
    if(m_pNextTrack == NULL) {
        emit needNextTrack();
    }

    if(enable) {
        qDebug() << "AutoDJ Enabled";
        // Begin AutoDJ...
        if (m_pCOPlay1->get() == 1.0f && m_pCOPlay2->get() == 1.0f) {
            qDebug() << "One player must be stopped before enabling Auto DJ mode";
            emit disableAutoDJ();
            return;
        }

        // Never load the same track if it is already playing
        if (m_pCOPlay1->get() == 1.0f) {
            emit removePlayingTrackFromQueue("[Channel1]");
        }
        if (m_pCOPlay2->get() == 1.0f) {
            emit removePlayingTrackFromQueue("[Channel2]");
        }

        m_bEnabled = enable;

        // Right now we are just using deck 1 and 2, in the future this will be
        // determined by which decks the user wants to use for AutoDJ
        connect(m_pCOPlayPosSamples1, SIGNAL(valueChanged(double)),
        this, SLOT(player1PositionChanged(double)));
        connect(m_pCOPlayPosSamples2, SIGNAL(valueChanged(double)),
        this, SLOT(player2PositionChanged(double)));

        m_bPlayer1Primed = false;
        m_bPlayer2Primed = false;
        m_bPlayer1Cued = false;
        m_bPlayer2Cued = false;

        // If only one of the players is playing...
        if ((m_pCOPlay1->get() == 1.0f && m_pCOPlay2->get() == 0.0f) ||
            (m_pCOPlay1->get() == 0.0f && m_pCOPlay2->get() == 1.0f)) {

            // Load the first song from the queue.
            if (m_bEndOfPlaylist) {
                // Queue is empty. Disable and return.
                emit disableAutoDJ();
                return;
            }
            loadNextTrack();

            // Set the primed flags so the crossfading algorithm knows
            // that it doesn't need to load a track into whatever player.
            if (m_pCOPlay1->get() == 1.0f) {
                m_bPlayer1Primed = true;
            }
            if (m_pCOPlay2->get() == 1.0f) {
                m_bPlayer2Primed = true;
            }
        }

        // If both players are stopped, start the first one
        else if (m_pCOPlay1->get() == 0.0f && m_pCOPlay2->get() == 0.0f) {
            // Load the first song from the queue.
            if (m_bEndOfPlaylist) {
                // Queue was empty. Disable and return.
                emit disableAutoDJ();
                return;
            }
            loadNextTrack();

            m_pCORepeat1->slotSet(1.0f); // Turn on repeat mode to avoid race condition between async load
            m_pCOPlay1->slotSet(1.0f);  // Play the track in player 1
            m_pTrackTransition->m_pCOCrossfader->slotSet(-1.0f);

            // Remove the track in Player 1 from the queue
           //emit removePlayingTrackFromQueue("[Channel1]");
        }
    }
    else {
        // Disable AutoDJ
        emit disableAutoDJ();
        qDebug() << "Auto DJ disabled";
        m_bEnabled = false;
        m_pCOPlayPosSamples1->disconnect(this);
        m_pCOPlayPosSamples2->disconnect(this);
        m_pCORepeat1->slotSet(0.0f); //Turn off repeat mode
        m_pCORepeat2->slotSet(0.0f); //Turn off repeat mode
    }
}

void AutoDJ::player1PositionChanged(double samplePos1) {}
/*
    TrackPointer pTrack = PlayerInfo::Instance().getTrackInfo("[Channel1]");

    emit removePlayingTrackFromQueue("[Channel1]");

    // Check if the other player already has a track loaded
    if(!m_bPlayer2Primed) {
        // Check if our next track is a duplicate of the track in Player 1
        if (pTrack->getTitle() == m_pNextTrack->getTitle()) {
            emit needNextTrack();
            return;
        }
        // Load the next track into the other player
        if (!m_bEndOfPlaylist) {
            loadNextTrack();
            m_bPlayer2Primed = true;
        }
    }

    // Check if Player 1 position is past the Fade Out point
    if (true){//samplePos1 > pTrack->getFadeOut()) {

        // Remove the track in Player 2 from the AutoDJ queue
        emit removePlayingTrackFromQueue("[Channel2]");

        m_pTrackTransition->transition("[Channel1]", "[Channel2]", pTrack);

        // Cue the track in Player 2 to the Fade In point
        if (m_bPlayer2Cued == false) {
            //cueTrackToFadeIn("[Channel2]");
            m_bPlayer2Cued = true;
        }

        // Check if other player is stopped
        if (m_pCOPlay2->get() == 0.0f) {
            // Turn off repeat mode in Player 1
            m_pCORepeat1->slotSet(0.0f);

            // Turn on repeat mode so Player 2 will start playing when the new track is loaded.
            // This helps us get around the fact that it takes time for the track to be loaded
            // and that is executed asynchronously (this gets around the race condition).
            m_pCORepeat2->slotSet(1.0f);
            // Play Player 2
            m_pCOPlay2->slotSet(1.0f);
        }

        // Check if the crossfade has finished
        if (m_pTrackTransition->m_pCOCrossfader->get() >= 1.0f) {
            m_pCOPlay1->slotSet(0.0f);
            m_bPlayer1Primed = false;
            m_bPlayer1Cued = false;
        }
    }
}*/

void AutoDJ::player2PositionChanged(double samplePos2) {}
/*
    TrackPointer pTrack = PlayerInfo::Instance().getTrackInfo("[Channel2]");

    emit removePlayingTrackFromQueue("[Channel2]");

    // Check if the other player already has a track loaded
    if (!m_bPlayer1Primed) {
        // Check if our next track is a duplicate of the track in Player 2
        if (pTrack->getTitle() == m_pNextTrack->getTitle()) {
            emit needNextTrack();
            return;
        }
        // Load the next track into the other player
        if (!m_bEndOfPlaylist) {
            loadNextTrack();
            m_bPlayer1Primed = true;
        }
    }

    // Check if Player 2 position is past the Fade Out point
    if (true){//samplePos2 > pTrack->getFadeOut()) {

        // Remove the track in Player 1 from the AutoDJ queue
        emit removePlayingTrackFromQueue("[Channel1");

        m_pTrackTransition->transition("[Channel2]", "[Channel1]", pTrack);

        // Cue the track in Player 1 to the Fade In point
        if (m_bPlayer1Cued == false) {
            cueTrackToFadeIn("[Channel1]");
            m_bPlayer1Cued = true;
        }

        // Check if other player is stopped
        if (m_pCOPlay1->get() == 0.0f) {
            // Turn off repeat mode in Player 2
            m_pCORepeat2->slotSet(0.0f);

            // Turn on repeat mode so Player 1 will start playing when the new track is loaded.
            // This helps us get around the fact that it takes time for the track to be loaded
            // and that is executed asynchronously (this gets around the race condition).
            m_pCORepeat1->slotSet(1.0f);
            // Play Player 1
            m_pCOPlay1->slotSet(1.0f);
        }

        // Check if the crossfade has finished
        if (m_pTrackTransition->m_pCOCrossfader->get() <= -1.0f) {
            m_pCOPlay2->slotSet(0.0f);
            m_bPlayer2Primed = false;
            m_bPlayer2Cued = false;
        }
    }
}*/

void AutoDJ::player1PlayChanged(double value) {}

void AutoDJ::player2PlayChanged(double value) {}

void AutoDJ::receiveNextTrack(TrackPointer nextTrack) {
    m_pNextTrack = nextTrack;
    qDebug() << "Received track from DlgAutoDJ: " << nextTrack->getTitle();
}

void AutoDJ::setEndOfPlaylist(bool endOfPlaylist) {
    m_bEndOfPlaylist = endOfPlaylist;
}

void AutoDJ::loadNextTrack() {
    emit loadTrack(m_pNextTrack);
    emit needNextTrack();
}

void AutoDJ::cueTrackToFadeIn(QString group) {

    TrackPointer pTrack = PlayerInfo::Instance().getTrackInfo(group);

    if (group == "[Channel1]") {
        // Get the FadeIn point of the track (in samples)
        double fadeIn = 0.0;//pTrack->getFadeIn();

        // Divide this by 2 and then by the sample rate to convert it to seconds
        fadeIn = (fadeIn/2)/(m_pCOSampleRate1->get());

        // Subtract the crossfade length
        fadeIn = fadeIn - (m_pTrackTransition->m_iFadeLength);

        // Check if the crossfade length set our cue to negative
        if (fadeIn < 0) {
            fadeIn = 0;
        }
        // Convert fadeIn to a ratio of the whole track
        fadeIn = fadeIn/(pTrack->getDuration());

        // Cue up the fadeIn point
        m_pTrackTransition->m_pCOPlayPos1->slotSet(fadeIn);
    }

    if (group == "[Channel2]") {
        // Get the FadeIn point of the track (in samples)
        double fadeIn = 0.0; //pTrack->getFadeIn();

        // Divide this by 2 and then by the sample rate to convert it to seconds
        fadeIn = (fadeIn/2)/(m_pCOSampleRate2->get());

        // Subtract the crossfade length
        fadeIn = fadeIn - (m_pTrackTransition->m_iFadeLength);

        // Check if the crossfade length set our cue to negative
        if (fadeIn < 0) {
            fadeIn = 0;
        }
        // Convert fadeIn to a ratio of the whole track
        fadeIn = fadeIn/(pTrack->getDuration());

        // Cue up the fadeIn point
        m_pTrackTransition->m_pCOPlayPos2->slotSet(fadeIn);
    }
}

    void AutoDJ::transitionValueChanged(int value) {
    	/*
        if (m_eState == ADJ_IDLE) {
            if (m_pCOPlay1Fb->get() == 1.0f) {
                player1PlayChanged(1.0f);
            }
            if (m_pCOPlay2Fb->get() == 1.0f) {
                player2PlayChanged(1.0f);
            }
        }
        m_pConfig->set(ConfigKey(CONFIG_KEY, kTransitionPreferenceName),
                       ConfigValue(value));
                       */
    }

void AutoDJ::shufflePlaylist(double value) {
	if (value == 1.0) return;
   	qDebug() << "Shuffling AutoDJ playlist";
   	int row;
   	if(m_eState == ADJ_DISABLED) {
   	    row = 0;
   	} else {
   	    row = 1;
   	}
   	m_pAutoDJTableModel->shuffleTracks(m_pAutoDJTableModel->index(row, 0));
   	qDebug() << "Shuffling done";
}

    void AutoDJ::skipNext(double value) {
    	/*
        Q_UNUSED(buttonChecked);
        qDebug() << "Skip Next";
        // Load the next song from the queue.
        if (m_pCOPlay1Fb->get() == 0.0f) {
            removePlayingTrackFromQueue("[Channel1]");
            loadNextTrackFromQueue();
        } else if (m_pCOPlay2Fb->get() == 0.0f) {
            removePlayingTrackFromQueue("[Channel2]");
            loadNextTrackFromQueue();
        }
        */
    }

    void AutoDJ::fadeNowRight(double value) {
    	/*
        Q_UNUSED(buttonChecked);
        qDebug() << "Fade Now";
        if (m_eState == ADJ_IDLE) {
            m_bFadeNow = true;
            double crossfader = m_pCOCrossfader->get();
            if (crossfader <= 0.3f && m_pCOPlay1Fb->get() == 1.0f) {
                m_posThreshold1 = m_pCOPlayPos1->get() -
                        ((crossfader + 1.0f) / 2 * (m_fadeDuration1));
                // Repeat is disabled by FadeNow but disables auto Fade
                m_pCORepeat1->slotSet(0.0f);
            } else if (crossfader >= -0.3f && m_pCOPlay2Fb->get() == 1.0f) {
                m_posThreshold2 = m_pCOPlayPos2->get() -
                        ((1.0f - crossfader) / 2 * (m_fadeDuration2));
                // Repeat is disabled by FadeNow but disables auto Fade
                m_pCORepeat2->slotSet(0.0f);
            }
        }
        */
    }

    void AutoDJ::fadeNowLeft(double value) {
    	/*
        Q_UNUSED(buttonChecked);
        qDebug() << "Fade Now";
        if (m_eState == ADJ_IDLE) {
            m_bFadeNow = true;
            double crossfader = m_pCOCrossfader->get();
            if (crossfader <= 0.3f && m_pCOPlay1Fb->get() == 1.0f) {
                m_posThreshold1 = m_pCOPlayPos1->get() -
                        ((crossfader + 1.0f) / 2 * (m_fadeDuration1));
                // Repeat is disabled by FadeNow but disables auto Fade
                m_pCORepeat1->slotSet(0.0f);
            } else if (crossfader >= -0.3f && m_pCOPlay2Fb->get() == 1.0f) {
                m_posThreshold2 = m_pCOPlayPos2->get() -
                        ((1.0f - crossfader) / 2 * (m_fadeDuration2));
                // Repeat is disabled by FadeNow but disables auto Fade
                m_pCORepeat2->slotSet(0.0f);
            }
        }
        */
    }

    void AutoDJ::toggleAutoDJ(double value) {
    	/*
        bool deck1Playing = m_pCOPlay1Fb->get() == 1.0f;
        bool deck2Playing = m_pCOPlay2Fb->get() == 1.0f;

        if (toggle) {  // Enable Auto DJ
            if (deck1Playing && deck2Playing) {
                QMessageBox::warning(
                    NULL, tr("Auto-DJ"),
                    tr("One deck must be stopped to enable Auto-DJ mode."),
                    QMessageBox::Ok);
                qDebug() << "One deck must be stopped before enabling Auto DJ mode";
                pushButtonAutoDJ->setChecked(false);
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
                pushButtonAutoDJ->setChecked(false);
                return;
            }

            // Track is available so GO
            pushButtonAutoDJ->setToolTip(tr("Disable Auto DJ"));
            pushButtonAutoDJ->setText(tr("Disable Auto DJ"));
            qDebug() << "Auto DJ enabled";

            pushButtonSkipNext->setEnabled(true);

            connect(m_pCOPlayPos1, SIGNAL(valueChanged(double)),
                    this, SLOT(player1PositionChanged(double)));
            connect(m_pCOPlayPos2, SIGNAL(valueChanged(double)),
                    this, SLOT(player2PositionChanged(double)));

            connect(m_pCOPlay1Fb, SIGNAL(valueChanged(double)),
                    this, SLOT(player1PlayChanged(double)));
            connect(m_pCOPlay2Fb, SIGNAL(valueChanged(double)),
                    this, SLOT(player2PlayChanged(double)));

            if (!deck1Playing && !deck2Playing) {
                // both decks are stopped
                m_eState = ADJ_ENABLE_P1LOADED;
                pushButtonFadeNow->setEnabled(false);
                // Force Update on load Track
                m_pCOPlayPos1->slotSet(-0.001f);
            } else {
                m_eState = ADJ_IDLE;
                pushButtonFadeNow->setEnabled(true);
                if (deck1Playing) {
                    // deck 1 is already playing
                    player1PlayChanged(1.0f);
                } else {
                    // deck 2 is already playing
                    player2PlayChanged(1.0f);
                }
            }
            // Loads into first deck If stopped else into second else not
            emit(loadTrack(nextTrack));
        } else {  // Disable Auto DJ
            pushButtonAutoDJ->setToolTip(tr("Enable Auto DJ"));
            pushButtonAutoDJ->setText(tr("Enable Auto DJ"));
            qDebug() << "Auto DJ disabled";
            m_eState = ADJ_DISABLED;
            pushButtonFadeNow->setEnabled(false);
            pushButtonSkipNext->setEnabled(false);
            m_bFadeNow = false;
            m_pCOPlayPos1->disconnect(this);
            m_pCOPlayPos2->disconnect(this);
            m_pCOPlay1->disconnect(this);
            m_pCOPlay2->disconnect(this);
        }
        */
    }

