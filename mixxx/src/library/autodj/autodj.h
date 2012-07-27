#ifndef AUTODJ_H
#define AUTODJ_H

#include <QObject>

#include "dlgautodj.h"
#include "library/autodj/tracktransition.h"
#include "controlpushbutton.h"
#include "library/dao/playlistdao.h"
#include "library/trackcollection.h"
#include "library/playlisttablemodel.h"
#include "trackinfoobject.h"

class ControlObjectThreadMain;
class DlgAutoDJ;
class TrackTransition;

class AutoDJ : public QObject {
    Q_OBJECT

public:
    AutoDJ(QObject* parent, ConfigObject<ConfigValue>* pConfig,
    		TrackCollection* pTrackCollection);
    ~AutoDJ();

    PlaylistTableModel* getTableModel();
    void setDlgAutoDJ(DlgAutoDJ* pDlgAutoDJ);

public slots:
    //void setEnabled(bool);
    void setEndOfPlaylist(bool);
    // Slot for Transition Combo Box
    void transitionSelect(int index);
    void receiveNextTrack(TrackPointer nextTrack);
    void player1PositionChanged(double samplePos1);
    void player2PositionChanged(double samplePos2);
    void player1PlayChanged(double value);
    void player2PlayChanged(double value);
    void transitionValueChanged(int value);
    void shufflePlaylist(double value);
    void skipNext(double value);
    void fadeNowRight(double value);
    void fadeNowLeft(double value);
    void toggleAutoDJ(double value);
    void setCueOut1(double value);
    void setCueOut2(double value);
    void deleteCueOut1(double value);
    void deleteCueOut2(double value);
    void setTransitionDone();
    void cueOutPos1(double value);
    void cueOutPos2(double value);

signals:
    // Emitted whenever AutoDJ is ready for the next track from the queue.
    // This is connected to DlgAutoDJ::slotNextTrackNeeded() by AutoDJFeature.
    // AutoDJ is going to control this now, not send a signal (smstewart)
    void needNextTrack();
    // Emitted to signal that the AutoDJ automation is stopping.
    // This is connected to DlgAutoDJ::toggleAutoDJ(bool) by AutoDJFeature.
    // This will now be controlled by COtoggleAutoDJ (smstewart)
    void disableAutoDJ();
    // Emitted so the currently playing track in the specified group will be removed from
    // the AutoDJ queue. This is connected to DlgAutoDJ::slotRemovePlayingTrackFromQueue(QString).
    void loadTrack(TrackPointer tio);
    void loadTrackToPlayer(TrackPointer tio, QString group);

private:
    DlgAutoDJ* m_pDlgAutoDJ;
    enum ADJstates {
        ADJ_IDLE = 0,
        ADJ_P1FADING,
        ADJ_P2FADING,
        ADJ_ENABLE_P1LOADED,
        ADJ_ENABLE_P1PLAYING,
        ADJ_DISABLED,
        ADJ_WAITING,
        ADJ_FADENOW
    };
    enum ADJstates m_eState;
    enum TranSelect {
    	CUE = 0,
    	BEAT,
    	CD
    };
    enum TranSelect m_eTransition;
    bool m_bEnabled;
    bool m_bEndOfPlaylist;
    bool m_btransitionDone;
    TrackCollection* m_pTrackCollection;
    PlaylistDAO& m_playlistDao;
    PlaylistTableModel* m_pAutoDJTableModel;
    // These variables (PlayerN) may be changed due to implementation,
    // but I am leaving them for now until I need to change them (smstewart)
    bool m_bPlayer1Primed, m_bPlayer2Primed;
    // TODO(tom__m) This is hacky
    bool m_bPlayer1Cued, m_bPlayer2Cued;
    // TODO(smstewart): Find better way to block out double keyboard signals
    // Checks for a double signal sent by keyboard press
    double lastToggleValue;
    int transitionValue;
    int m_iCueRecall;

    ConfigObject<ConfigValue>* m_pConfig;
    ControlObject* m_pCOCueOutPosition1;
    ControlObject* m_pCOCueOutPosition2;
    ControlObjectThreadMain* m_pCOPlay1;
    ControlObjectThreadMain* m_pCOPlay2;
    ControlObjectThreadMain* m_pCOPlay1Fb;
    ControlObjectThreadMain* m_pCOPlay2Fb;
    ControlObjectThreadMain* m_pCORepeat1;
    ControlObjectThreadMain* m_pCORepeat2;
    ControlObjectThreadMain* m_pCOPlayPos1;
    ControlObjectThreadMain* m_pCOPlayPos2;
    ControlObjectThreadMain* m_pCOSampleRate1;
    ControlObjectThreadMain* m_pCOSampleRate2;
    ControlObjectThreadMain* m_pCOTrackSamples1;
    ControlObjectThreadMain* m_pCOTrackSamples2;
    ControlObjectThreadMain* m_pCOCrossfader;
    ControlObjectThreadMain* m_pCOSync;
    ControlObjectThreadMain* m_pPitchControl;
    ControlObjectThreadMain* m_pKeyLock;
    ControlObjectThreadMain* m_pCOToggleAutoDJThread;
    ControlObjectThreadMain* m_pCOSkipNextThread;
    ControlPushButton* m_pCOSkipNext;
    ControlPushButton* m_pCOFadeNowRight;
    ControlPushButton* m_pCOFadeNowLeft;
    ControlPushButton* m_pCOShufflePlaylist;
    ControlPushButton* m_pCOToggleAutoDJ;
    ControlPushButton* m_pCOSetCueOut1;
    ControlPushButton* m_pCOSetCueOut2;
    ControlPushButton* m_pCODeleteCueOut1;
    ControlPushButton* m_pCODeleteCueOut2;

    float m_fadeDuration1, m_fadeDuration2, m_posThreshold1, m_posThreshold2;
    // This is effectively our "on deck" track for AutoDJ.
    TrackPointer m_pNextTrack;

    // The TrackTransition object is created by AutoDJ to handle the crossfading
    // algorithm as well as any other logic related to the transition between the
    // tracks.
    TrackTransition* m_pTrackTransition;

    // Handles the emission of signals needed to load the next track as well as
    // signal that AutoDJ needs a new track.
    void loadNextTrack();

    // Cues the track in the specified player to the X seconds before the FadeIn point
    // where X is the user defined crossfade length
    // Tracks are loaded at the default cue point, so AutoDJ doesn't need to
    // do anyting extra (smstewart)
    //void cueTrackToFadeIn(QString group);

    TrackPointer getNextTrackFromQueue();
    bool loadNextTrackFromQueue();
    bool removePlayingTrackFromQueue(QString group);
    void setCueOut(double position, int channel);
    void deleteCueOut(double value, int channel);
    void startAutoDJ();
};

#endif // AUTODJ_H
