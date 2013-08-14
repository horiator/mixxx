#ifndef TRACKTRANSITION_H
#define TRACKTRANSITION_H

#include <QObject>

#include "configobject.h"
#include "trackinfoobject.h"
#include "library/autodj/autodj.h"

class ControlObjectThreadMain;

class TrackTransition : public QObject {
    Q_OBJECT

public:
    TrackTransition(QObject* parent, ConfigObject<ConfigValue>* pConfig);
    virtual ~TrackTransition();
    void setGroups(QString groupA, QString groupB);
    void calculateCueOut();
    void loadTrack();
    bool m_bTrackToSynced;

public slots:
    void crossfaderChange(double value);
    void slotBpmChanged(double value);

private:
    double m_dcrossfadePosition;
    double m_dCurrentPlayPos;
    bool m_bTransitioning;
    bool m_bShortCue;
    bool m_bDeckBCue;
    bool m_bFadeNow;
    bool m_bSpinBack;
    QString m_groupFrom;
    QString m_groupTo;
    TrackPointer m_trackFrom;
    TrackPointer m_trackTo;
    TrackPointer* m_trackAPointer;
    TrackPointer* m_trackToPointer;
    double m_dBpmFrom;
    double m_dBpmTo;
    double m_dBpmShift;
    bool m_bTrackLoaded;
    int m_iCurrentPos;
    int m_iShortCue;
    int m_cueOutPoint;
    int m_latestCueOutPoint;
    int m_iFadeStart;
    int m_iFadeEnd;
    int m_iFadeLength;
    double m_dCrossfaderStart;
    double m_dBrakeRate;
    double m_dSpinRate;
    void transitioning();
    void calculateShortCue();
    void calculateDeckBCue();
    void fadeNowStopped();
    bool cueTransition(double value);
    bool cdTransition(double value);
    bool beatTransition(double value);
    void fadeNowTransition(double value);
    void brakeTransition(double value);
    void spinBackTransition(double value);

    void fadeNowRight();
    void fadeNowLeft();

    ConfigObject<ConfigValue>* m_pConfig;
    ControlObjectThreadMain* m_pCOCrossfader;
    ControlObjectThreadMain* m_pCOPlayPos1;
    ControlObjectThreadMain* m_pCOPlayPos2;
    ControlObjectThreadMain* m_pCOPlay1;
    ControlObjectThreadMain* m_pCOPlay2;
    ControlObjectThreadMain* m_pCOTrackSamples1;
    ControlObjectThreadMain* m_pCOTrackSamples2;
    ControlObjectThreadMain* m_pCOSync1;
    ControlObjectThreadMain* m_pCOSync2;
    ControlObjectThreadMain* m_pCOSyncPhase1;
    ControlObjectThreadMain* m_pCOSyncPhase2;
    ControlObjectThreadMain* m_pCOSyncTempo1;
    ControlObjectThreadMain* m_pCOSyncTempo2;
    ControlObjectThreadMain* m_pCOCueOut1;
    ControlObjectThreadMain* m_pCOCueOut2;
    ControlObjectThreadMain* m_pCOFadeNowLeft;
    ControlObjectThreadMain* m_pCOFadeNowRight;
    ControlObjectThreadMain* m_pScratch1;
    ControlObjectThreadMain* m_pScratch2;
    ControlObjectThreadMain* m_pScratchEnable1;
    ControlObjectThreadMain* m_pScratchEnable2;
    ControlObjectThreadMain* m_pCORate1;
    ControlObjectThreadMain* m_pCORate2;
    ControlObjectThreadMain* m_pCOBpm1;
    ControlObjectThreadMain* m_pCOBpm2;
    ControlObjectThreadMain* m_pCOCueGoTo1;
    ControlObjectThreadMain* m_pCOCueGoTo2;

    friend class AutoDJ;

};

#endif // TRACKTRANSITION_H
