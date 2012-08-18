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
    void transitionSelect(int index);
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

signals:
    void loadTrack(TrackPointer tio);
    void loadTrackToPlayer(TrackPointer tio, QString group);

private:
    ConfigObject<ConfigValue>* m_pConfig;
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
    bool m_btransitionDone;
    TrackCollection* m_pTrackCollection;
    PlaylistDAO& m_playlistDao;
    PlaylistTableModel* m_pAutoDJTableModel;
    // Checks for a double signal sent by keyboard press
    double lastToggleValue;
    int transitionValue;
    int m_iCueRecall;

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
    ControlObjectThreadMain* m_pCOToggleAutoDJThread;
    ControlObjectThreadMain* m_pCOSkipNextThread;
    ControlObjectThreadMain* m_pCOFadeNowRightThread;
    ControlObjectThreadMain* m_pCOFadeNowLeftThread;
    ControlObjectThreadMain* m_pCOScratch1;
    ControlObjectThreadMain* m_pCOScratch2;
    ControlObjectThreadMain* m_pCOScratchEnable1;
    ControlObjectThreadMain* m_pCOScratchEnable2;
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

    // The TrackTransition object is created by AutoDJ to handle the crossfading
    // methods as well as any other logic related to the transition between the
    // tracks.
    TrackTransition* m_pTrackTransition;

    TrackPointer getNextTrackFromQueue();
    bool loadNextTrackFromQueue();
    bool removePlayingTrackFromQueue(QString group);
    void setCueOut(double position, int channel);
    void deleteCueOut(double value, int channel);
};

#endif // AUTODJ_H
