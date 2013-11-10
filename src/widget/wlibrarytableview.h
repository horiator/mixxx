// wlibrarytableview.h
// Created 10/19/2009 by RJ Ryan (rryan@mit.edu)

#ifndef WLIBRARYTABLEVIEW_H
#define WLIBRARYTABLEVIEW_H

#include <QString>
#include <QTableView>

#include "configobject.h"
#include "library/abstractlibraryview.h"
#include "trackinfoobject.h"


class WLibraryTableView : public QTableView, public virtual AbstractLibraryView {
    Q_OBJECT

  public:
    WLibraryTableView(QWidget* parent,
                      ConfigObject<ConfigValue>* pConfig,
                      ConfigKey vScrollBarPosKey);
    virtual ~WLibraryTableView();
    virtual void moveSelection(int delta);

  signals:
    void loadTrack(TrackPointer pTrack);
    void loadTrackToPlayer(TrackPointer pTrack, QString group, bool play = false);
    void coverChanged(QString img);

  public slots:
    void saveVScrollBarPos();
    void restoreVScrollBarPos();

  private:
    void loadVScrollBarPosState();
    void saveVScrollBarPosState();

    ConfigObject<ConfigValue>* m_pConfig;
    ConfigKey m_vScrollBarPosKey;
    // The position of the vertical scrollbar slider, eg. before a search is
    // executed
    int m_iSavedVScrollBarPos;
};


#endif /* WLIBRARYTABLEVIEW_H */
