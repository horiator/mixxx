// AutoDJfeature.h
// FORK FORK FORK on 11/1/2009 by Albert Santoni (alberts@mixxx.org)
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)

#ifndef AUTODJFEATURE_H
#define AUTODJFEATURE_H

#include <QStringListModel>

#include "library/libraryfeature.h"
#include "library/dao/playlistdao.h"
#include "configobject.h"
#include "library/treeitemmodel.h"
#include "dlgautodj.h"
#include "library/autodj/autodj.h"

class PlaylistTableModel;
class TrackCollection;

class AutoDJFeature : public LibraryFeature {
    Q_OBJECT
  public:
    AutoDJFeature(QObject* parent,
                  ConfigObject<ConfigValue>* pConfig,
                  TrackCollection* pTrackCollection);
    virtual ~AutoDJFeature();

    QVariant title();
    QIcon getIcon();

    bool dropAccept(QList<QUrl> urls,QWidget *pSource);
    bool dragMoveAccept(QUrl url);

    void bindWidget(WLibrary* libraryWidget,
                    MixxxKeyboard* keyboard);

    TreeItemModel* getChildModel();

  public slots:
    void activate();

  private:
    ConfigObject<ConfigValue>* m_pConfig;
    TrackCollection* m_pTrackCollection;
    PlaylistDAO& m_playlistDao;
    const static QString m_sAutoDJViewName;
    TreeItemModel m_childModel;
		AutoDJ* m_pAutoDJ;
    DlgAutoDJ* m_pAutoDJView;
};


#endif /* AUTODJFEATURE_H */
