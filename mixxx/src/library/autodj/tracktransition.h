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
    void setTransitionDoneFalse();

public slots:
	void checkForUserInput(double value);

signals:
	void transitionDone();

private:
	double m_dcrossfadePosition;
    enum m_etransitionSelect {
    	CD = 0,
    	CUE
    };
    bool m_buserTakeOver;
    bool m_btransitionDone;
    QString m_groupA;
    QString m_groupB;
    TrackPointer m_trackA;
    TrackPointer m_trackB;
    //int m_iTrackACue;
    int m_icuePoint;
    //int m_iTrackBCue;
    // This is the transition function used by AutoDJ to crossfade from the desired
    // deckA to deckB
    void cueTransition(double value);
    void cdTransition(double value);
    // The user defined crossfade length
    int m_iFadeLength;

    ConfigObject<ConfigValue>* m_pConfig;
    ControlObjectThreadMain* m_pCOCrossfader;
    ControlObjectThreadMain* m_pCOPlayPos1;
    ControlObjectThreadMain* m_pCOPlayPos2;
    ControlObjectThreadMain* m_pCOPlay1;
    ControlObjectThreadMain* m_pCOPlay2;
    ControlObjectThreadMain* m_pCOTrackSamples1;
    ControlObjectThreadMain* m_pCOTrackSamples2;

    friend class AutoDJ;

};

#endif // TRACKTRANSITION_H
