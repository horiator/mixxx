#include <QSqlTableModel>
#include "widget/wwidget.h"
#include "widget/wskincolor.h"
#include "widget/wtracktableview.h"
#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "library/trackcollection.h"
#include "library/playlisttablemodel.h"
#include "playerinfo.h"
#include "dlgautodj.h"

#define CONFIG_KEY "[Auto DJ]"

DlgAutoDJ::DlgAutoDJ(QWidget* parent, ConfigObject<ConfigValue>* pConfig,
                     TrackCollection* pTrackCollection, MixxxKeyboard* pKeyboard)
     : QWidget(parent), Ui::DlgAutoDJ(), m_playlistDao(pTrackCollection->getPlaylistDAO())
{
    setupUi(this);

    m_pConfig = pConfig;
    m_pTrackCollection = pTrackCollection;
    m_bAutoDJEnabled = false;
	m_bFadeNow = false;
	m_eState = ADJ_IDLE;

	m_posThreshold1 = 1.0f;
	m_posThreshold2 = 1.0f;

    m_pTrackTableView = new WTrackTableView(this, pConfig, m_pTrackCollection);
    m_pTrackTableView->installEventFilter(pKeyboard);

    connect(m_pTrackTableView, SIGNAL(loadTrack(TrackPointer)),
            this, SIGNAL(loadTrack(TrackPointer)));
    connect(m_pTrackTableView, SIGNAL(loadTrackToPlayer(TrackPointer, QString)),
            this, SIGNAL(loadTrackToPlayer(TrackPointer, QString)));

    QBoxLayout* box = dynamic_cast<QBoxLayout*>(layout());
    Q_ASSERT(box); //Assumes the form layout is a QVBox/QHBoxLayout!
    box->removeWidget(m_pTrackTablePlaceholder);
    m_pTrackTablePlaceholder->hide();
    box->insertWidget(1, m_pTrackTableView);

    m_pAutoDJTableModel =  new PlaylistTableModel(this, pTrackCollection);
    int playlistId = m_playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);
    if (playlistId < 0) {
        m_playlistDao.createPlaylist(AUTODJ_TABLE, PlaylistDAO::PLHT_AUTO_DJ);
        playlistId = m_playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);
    }
    m_pAutoDJTableModel->setPlaylist(playlistId);
    m_pTrackTableView->loadTrackModel(m_pAutoDJTableModel);

    //Override some playlist-view properties:
    //Prevent drag and drop to the waveform or elsewhere so you can't preempt the Auto DJ queue...
    m_pTrackTableView->setDragDropMode(QAbstractItemView::InternalMove);
    //Sort by the position column and lock it
    m_pTrackTableView->sortByColumn(0, Qt::AscendingOrder);
    // m_pTrackTableView->setSortingEnabled(false); // Bug: disables only the sorting arrow
    												// Bud anyway, why not allow sorting?

    connect(pushButtonShuffle, SIGNAL(clicked(bool)),
            this, SLOT(shufflePlaylist(bool)));

    connect(pushButtonSkipNext, SIGNAL(clicked(bool)),
            this, SLOT(skipNext(bool)));

    connect(pushButtonFadeNow, SIGNAL(clicked(bool)),
            this, SLOT(fadeNow(bool)));

    connect(spinBoxTransition, SIGNAL(valueChanged(int)),
    this, SLOT(transitionValueChanged(int)));

    connect(pushButtonAutoDJ, SIGNAL(toggled(bool)),
            this,  SLOT(toggleAutoDJ(bool))); _blah;

    // playposition is from -0.14 to + 1.14
    m_pCOPlayPos1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "playposition")));
    m_pCOPlayPos2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "playposition")));
    m_pCOPlay1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "play")));
    m_pCOPlay2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "play")));
    m_pCOPlay1Fb = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "play")));
    m_pCOPlay2Fb = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "play")));
    m_pCORepeat1 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel1]", "repeat")));
    m_pCORepeat2 = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Channel2]", "repeat")));
    m_pCOCrossfader = new ControlObjectThreadMain(
                            ControlObject::getControl(ConfigKey("[Master]", "crossfader")));

    QString str_autoDjTransition = m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "Transition"));
	if (str_autoDjTransition.isEmpty()) {
		spinBoxTransition->setValue(10);  // default 10 sec
	}
	else {
		spinBoxTransition->setValue(str_autoDjTransition.toInt());
	}
}

DlgAutoDJ::~DlgAutoDJ()
{
    delete m_pCOPlayPos1;
    delete m_pCOPlayPos2;
    delete m_pCOPlay1;
    delete m_pCOPlay2;
	delete m_pCORepeat1;
    delete m_pCORepeat2;
    delete m_pCOCrossfader;
    delete m_pAutoDJTableModel;
}

void DlgAutoDJ::onShow()
{
    m_pAutoDJTableModel->select();
}

void DlgAutoDJ::setup(QDomNode node)
{

    QPalette pal = palette();

    // Row colors
    if (!WWidget::selectNode(node, "BgColorRowEven").isNull() &&
        !WWidget::selectNode(node, "BgColorRowUneven").isNull()) {
        QColor r1;
        r1.setNamedColor(WWidget::selectNodeQString(node, "BgColorRowEven"));
        r1 = WSkinColor::getCorrectColor(r1);
        QColor r2;
        r2.setNamedColor(WWidget::selectNodeQString(node, "BgColorRowUneven"));
        r2 = WSkinColor::getCorrectColor(r2);

        // For now make text the inverse of the background so it's readable In
        // the future this should be configurable from the skin with this as the
        // fallback option
        QColor text(255 - r1.red(), 255 - r1.green(), 255 - r1.blue());

        //setAlternatingRowColors ( true );

        QColor fgColor;
        fgColor.setNamedColor(WWidget::selectNodeQString(node, "FgColor"));
        fgColor = WSkinColor::getCorrectColor(fgColor);

        pal.setColor(QPalette::Base, r1);
        pal.setColor(QPalette::AlternateBase, r2);
        pal.setColor(QPalette::Text, text);
        pal.setColor(QPalette::WindowText, fgColor);

    }

    setPalette(pal);

    pushButtonAutoDJ->setPalette(pal);
    //m_pTrackTableView->setPalette(pal); //Since we're getting this passed into us already created,
                                          //shouldn't need to set the palette.
}

void DlgAutoDJ::onSearchStarting()
{
}

void DlgAutoDJ::onSearchCleared()
{
}

void DlgAutoDJ::onSearch(const QString& text)
{
    m_pAutoDJTableModel->search(text);
}

void DlgAutoDJ::loadSelectedTrack() {
    m_pTrackTableView->loadSelectedTrack();
}

void DlgAutoDJ::loadSelectedTrackToGroup(QString group) {
    m_pTrackTableView->loadSelectedTrackToGroup(group);
}

void DlgAutoDJ::moveSelection(int delta) {
    m_pTrackTableView->moveSelection(delta);
}

void DlgAutoDJ::shufflePlaylist(bool buttonChecked)
{
    Q_UNUSED(buttonChecked);
    m_pTrackTableView->sortByColumn(0, Qt::AscendingOrder);
    qDebug() << "Shuffling AutoDJ playlist";
    m_pAutoDJTableModel->shuffleTracks(m_pAutoDJTableModel->index(0, 0));
    qDebug() << "Shuffling done";
}

void DlgAutoDJ::skipNext(bool buttonChecked) 
{
    Q_UNUSED(buttonChecked);
    //m_pTrackTableView->sortByColumn(0, Qt::AscendingOrder);
    qDebug() << "Skip Next";
    //Load the next song from the queue.

    if (m_pCOPlay1Fb->get() == 0.0f) {
    	removePlayingTrackFromQueue("[Channel1]");
    	loadNextTrackFromQueue();
    }
    else if (m_pCOPlay2Fb->get() == 0.0f) {
    	removePlayingTrackFromQueue("[Channel2]");
    	loadNextTrackFromQueue();
    }

}

void DlgAutoDJ::fadeNow(bool buttonChecked)
{
	Q_UNUSED(buttonChecked);
    qDebug() << "Fade Now";
	if (m_eState == ADJ_IDLE && m_bAutoDJEnabled) {		
		m_bFadeNow = true;
		double crossfader = m_pCOCrossfader->get();
		if (crossfader < -0.9f && m_pCOPlay1Fb->get() == 1.0f) {
			m_posThreshold1 = m_pCOPlayPos1->get();
			m_pCORepeat1->slotSet(0.0f); // Repeat is disabled by FadeNow but disables auto Fade
		}
		else if (crossfader > 0.9f && m_pCOPlay2Fb->get() == 1.0f) {
			m_posThreshold2 = m_pCOPlayPos2->get();
			m_pCORepeat2->slotSet(0.0f); // Repeat is disabled by FadeNow but disables auto Fade
		}
	}
}

void DlgAutoDJ::toggleAutoDJ(bool toggle)
{
	if (toggle) //Enable Auto DJ
    {
    	if (    m_pCOPlay1Fb->get() == 1.0f
        	 && m_pCOPlay2Fb->get() == 1.0f
        ){
            qDebug() << "One player must be stopped before enabling Auto DJ mode";
            pushButtonAutoDJ->setChecked(false);
            return;
        }

    	// Never load the same track if it is already playing
    	if (m_pCOPlay1Fb->get() == 1.0f) {
    		removePlayingTrackFromQueue("[Channel1]");
    	}
    	if ( m_pCOPlay2Fb->get() == 1.0f) {
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
		qDebug() << "Auto DJ enabled";
        m_bAutoDJEnabled = true;

        connect(m_pCOPlayPos1, SIGNAL(valueChanged(double)),
        this, SLOT(player1PositionChanged(double)));
        connect(m_pCOPlayPos2, SIGNAL(valueChanged(double)),
        this, SLOT(player2PositionChanged(double)));

        connect(m_pCOPlay1Fb, SIGNAL(valueChanged(double)),
        this, SLOT(player1PlayChanged(double)));
        connect(m_pCOPlay2Fb, SIGNAL(valueChanged(double)),
        this, SLOT(player2PlayChanged(double)));

        if (m_pCOPlay1Fb->get() == 0.0f && m_pCOPlay2Fb->get() == 0.0f) {
        	m_eState = ADJ_ENABLE_P1LOADED;
        	m_pCOPlayPos1->slotSet(-0.001f); // Force Update on load Track
        }
		else if (m_pCOPlay1Fb->get() == 0.0f)
		{
			m_eState = ADJ_IDLE;
	        player1PlayChanged(1.0f);
		}
		else {
			m_eState = ADJ_IDLE;
	        player2PlayChanged(1.0f);
		}
        emit(loadTrack(nextTrack)); // Loads into first deck If stopped else into second else not
    }
    else { //Disable Auto DJ
        pushButtonAutoDJ->setToolTip(tr("Enable Auto DJ"));
        qDebug() << "Auto DJ disabled";
        m_bAutoDJEnabled = false;
		m_bFadeNow = false;
        m_pCOPlayPos1->disconnect(this);
        m_pCOPlayPos2->disconnect(this);
        m_pCOPlay1->disconnect(this);
        m_pCOPlay2->disconnect(this);
    }
}

void DlgAutoDJ::player1PositionChanged(double value)
{
    // const float posThreshold = 0.95; //95% playback is when we crossfade and do stuff
    const float fadeDuration = m_fadeDuration1;  // 0.05; // 5% playback is crossfade duration

	// qDebug() << "player1PositionChanged(" << value << ")";

	if (!m_bAutoDJEnabled) {
		//nothing to do 
		return;			
	}


	if (m_eState == ADJ_ENABLE_P1LOADED) {	
		// Auto DJ Start
	    if (m_pCOPlay1Fb->get() == 0.0f && m_pCOPlay2Fb->get() == 0.0f) {		
            m_pCOCrossfader->slotSet(-1.0f); //Move crossfader to the left!
            m_pCOPlay1->slotSet(1.0f); //Play the track in player 1
			removePlayingTrackFromQueue("[Channel1]");
        }
		else if (m_pCOPlay1Fb->get() == 1.0f && m_pCOPlay2Fb->get() == 0.0f) {
			// Here we are, if first deck was playing before starting Auto DJ
			// or if it was started just before
            loadNextTrackFromQueue();			
			m_eState = ADJ_IDLE;
			player1PlayChanged(1.0f); // if we start the deck from code we don`t get a signal
									  // call function manually
		}
		else {
			m_eState = ADJ_IDLE;
			player2PlayChanged(1.0f);
		}
	    return;
	}

	if (m_eState == ADJ_P2FADING) {
		if (m_pCOPlay2Fb->get() == 0.0f && m_pCOPlay1Fb->get() == 1.0f) {
			loadNextTrackFromQueue();
			// End State
			m_pCOCrossfader->slotSet(-1.0f); //Move crossfader to the left!
			// qDebug() << "1: m_pCOCrossfader->slotSet(_-1.0f_);";
			m_eState = ADJ_IDLE;
		}
		return;
	}

	if (m_eState == ADJ_IDLE) {
		if (m_pCORepeat1->get() == 1.0f) {
			//repeat disables auto DJ
			return;
		}
	}

    if (value >= m_posThreshold1) {
		if (     m_eState == ADJ_IDLE
			 && (m_pCOPlay1Fb->get() == 1.0f|| m_posThreshold1 >= 1.0f)
		) {      
			if (m_pCOPlay2Fb->get() == 0.0f) {
		        // Start Deck 2
				player2PlayChanged(1.0f);
				m_pCOPlay2->slotSet(1.0f);
		        if (fadeDuration < 0.0f) {
		        	// Scroll back for pause between tracks
		        	m_pCOPlayPos2->slotSet(m_fadeDuration2);
		        }
		    }
			removePlayingTrackFromQueue("[Channel2]");

			m_eState = ADJ_P1FADING;
		}

		float posFadeEnd = m_posThreshold1 + fadeDuration;
		if( posFadeEnd > 1.0f ) posFadeEnd = 1.0f;
		
       if (value >= posFadeEnd) {
			// Pre-EndState			
			// m_pCOCrossfader->slotSet(1.0f); //Move crossfader to the right!
            
			m_pCOPlay1->slotSet(0.0f); //Stop the player
            
			//m_posThreshold = 1.0f - fadeDuration; // back to default
								
			// does not work always emediatly after stop
			// loadNextTrackFromQueue();
			// m_eState = ADJ_IDLE; // Fading ready
        }
        else {
        	//Crossfade!
        	float crossfadeValue = -1.0f + 2*(value-m_posThreshold1)/(posFadeEnd-m_posThreshold1);
        	// crossfadeValue = -1.0f -> + 1.0f
        	m_pCOCrossfader->slotSet(crossfadeValue); //Move crossfader to the right!
        	// qDebug() << "1: m_pCOCrossfader->slotSet " << crossfadeValue;
        }
    }
}

void DlgAutoDJ::player2PositionChanged(double value)
{
    // const float posThreshold = 0.95; //95% playback is when we crossfade and do stuff
    float fadeDuration = m_fadeDuration2; // 0.05; // 5% playback is crossfade duration

	//qDebug() << "player2PositionChanged(" << value << ")";


	if (!m_bAutoDJEnabled) {
		//nothing to do 
		return;			
	}

	if (m_eState == ADJ_P1FADING) {	
		if (m_pCOPlay1Fb->get() == 0.0f && m_pCOPlay2Fb->get() == 1.0f) {
			// End State
			m_pCOCrossfader->slotSet(1.0f); //Move crossfader to the right!
			// qDebug() << "1: m_pCOCrossfader->slotSet(_1.0f_);";
			m_eState = ADJ_IDLE;
			loadNextTrackFromQueue();
		}
		return;
	}

	if (m_eState == ADJ_IDLE) {	
		if (m_pCORepeat2->get() == 1.0f) {
			//repeat disables auto DJ
			return;
		}
	}

    if (value >= m_posThreshold2) {
		if(     m_eState == ADJ_IDLE
			&& (m_pCOPlay2Fb->get() == 1.0f|| m_posThreshold2 >= 1.0f)
		) {      
			if (m_pCOPlay1Fb->get() == 0.0f) {
				player1PlayChanged(1.0f);
				m_pCOPlay1->slotSet(1.0f);
		        if(fadeDuration < 0 ){
		        	// Scroll back for pause between tracks
		        	m_pCOPlayPos1->slotSet(m_fadeDuration1);
		        }
		    }
			removePlayingTrackFromQueue("[Channel1]");
			m_eState = ADJ_P2FADING;
		}

		float posFadeEnd = m_posThreshold2 + fadeDuration;
		if( posFadeEnd > 1.0f ) posFadeEnd = 1.0f;
		
        if (value >= posFadeEnd) {
			//Pre-End State			
			//m_pCOCrossfader->slotSet(-1.0f); //Move crossfader to the left!
            
			m_pCOPlay2->slotSet(0.0f); //Stop the player

			//m_posThreshold = 1.0f - fadeDuration; // back to default
			
			// does not work always immediately after stop
			// loadNextTrackFromQueue();
			// m_eState = ADJ_IDLE; // Fading ready
        }
		else {
			//Crossfade!
		    float crossfadeValue = 1.0f - 2*(value-m_posThreshold2)/(posFadeEnd-m_posThreshold2);
		    // crossfadeValue = 1.0f -> + -1.0f
			m_pCOCrossfader->slotSet(crossfadeValue); //Move crossfader to the right!
			// qDebug() << "2: m_pCOCrossfader->slotSet " << crossfadeValue;
		}
    }
}


TrackPointer DlgAutoDJ::getNextTrackFromQueue()
{
    //Get the track at the top of the playlist...

    TrackPointer nextTrack;

    for (;;) {
		nextTrack = m_pAutoDJTableModel->getTrack(m_pAutoDJTableModel->index(0, 0));

		if (nextTrack) {
			if (nextTrack->exists()) {
				// found a valid Track
				return nextTrack;
			}
			else {
				// Remove missing song from auto DJ playlist
				m_pAutoDJTableModel->removeTrack(m_pAutoDJTableModel->index(0, 0));
			}
		}
		else {
			// we are running out of tracks
			return nextTrack;
		}
    }
}


bool DlgAutoDJ::loadNextTrackFromQueue()
{
    //Get the track at the top of the playlist...

    TrackPointer nextTrack = getNextTrackFromQueue();

	if (!nextTrack) { //We ran out of tracks in the queue...
		//Disable auto DJ and return...
		pushButtonAutoDJ->setChecked(false);
		return false;
	}

	emit(loadTrack(nextTrack));
	return true;
}

bool DlgAutoDJ::removePlayingTrackFromQueue(QString group)
{
	TrackPointer nextTrack, loadedTrack;
	int nextId = 0, loadedId = 0;
	

	//Get the track at the top of the playlist...
    nextTrack = m_pAutoDJTableModel->getTrack(m_pAutoDJTableModel->index(0, 0));
	if (nextTrack) {
		nextId = nextTrack->getId();
	}	

	//Get loaded track
	loadedTrack = PlayerInfo::Instance().getTrackInfo(group);
	if (loadedTrack) {	
		loadedId = loadedTrack->getId();
	}

	//When enable auto DJ and Topmost Song is already on second deck, nothing to do  
	//  BaseTrackPlayer::getLoadedTrack()   
	//  pTrack = PlayerInfo::Instance().getCurrentPlayingTrack(); 

	
	if (loadedId != nextId) {
		// Do not remove when the user has loaded a track manualy
		return false;
    }

    // remove the top track 
	m_pAutoDJTableModel->removeTrack(m_pAutoDJTableModel->index(0, 0));	

    return true;
}

void DlgAutoDJ::player1PlayChanged(double value){
	qDebug() << "player1PlayChanged(" << value << ")";

	if (value == 1.0f && m_eState == ADJ_IDLE) {
		TrackPointer loadedTrack  = PlayerInfo::Instance().getTrackInfo("[Channel1]");
		if (loadedTrack) {
			int TrackDuration = loadedTrack->getDuration();
			qDebug() << "TrackDuration = " << TrackDuration;

			int autoDjTransition = spinBoxTransition->value();

			if (TrackDuration > autoDjTransition) {
				m_fadeDuration1 = (float)autoDjTransition / (float)TrackDuration;
			}
			else {
				m_fadeDuration1 = 0;
			}
			if (autoDjTransition > 0) {
				m_posThreshold1 = 1.0f - m_fadeDuration1;
			}
			else {
				// in case of pause
				m_posThreshold1 = 1.0f;
			}
			qDebug() << "m_fadeDuration1 = " << m_fadeDuration1;
		}
	}
}

void DlgAutoDJ::player2PlayChanged(double value){
	qDebug() << "player2PlayChanged(" << value << ")";

	if (value == 1.0f && m_eState == ADJ_IDLE) {
		TrackPointer loadedTrack  = PlayerInfo::Instance().getTrackInfo("[Channel2]");
		if (loadedTrack) {
			int TrackDuration = loadedTrack->getDuration();
			qDebug() << "TrackDuration = " << TrackDuration;

			int autoDjTransition = spinBoxTransition->value();

			if (TrackDuration > autoDjTransition) {
				m_fadeDuration2 = (float)autoDjTransition / (float)TrackDuration;
			}
			else {
				m_fadeDuration2 = 0;
			}
			if (autoDjTransition > 0) {
				m_posThreshold2 = 1.0f - m_fadeDuration2;
			}
			else {
				// in case of pause
				m_posThreshold2 = 1.0f;
			}
			qDebug() << "m_fadeDuration2 = " << m_fadeDuration2;
		}
	}
}

void DlgAutoDJ::transitionValueChanged(int value){

	if (m_bAutoDJEnabled) {
		if (m_bAutoDJEnabled && m_eState == ADJ_IDLE){
			if (m_pCOPlay1Fb->get() == 1.0f) {
				player1PlayChanged(1.0f);
			}
			if (m_pCOPlay2Fb->get() == 1.0f) {
				player2PlayChanged(1.0f);
			}
		}
	}
	m_pConfig->set(ConfigKey(CONFIG_KEY, "Transition"), ConfigValue(value));
}

bool DlgAutoDJ::appendTrack(int trackId){
	return m_pAutoDJTableModel->appendTrack(trackId);
}
