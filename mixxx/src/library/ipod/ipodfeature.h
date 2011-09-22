
#ifndef IPODFEATURE_H
#define IPODFEATURE_H

#include <QStringListModel>
#include <QtSql>
#include <QFuture>
#include <QtConcurrentRun>
#include <QFutureWatcher>

#include "library/libraryfeature.h"
#include "library/trackcollection.h"
#include "library/treeitemmodel.h"
#include "library/treeitem.h"

extern "C"
{
#include <gpod/itdb.h>
}




//class ITunesPlaylistModel;
class ITunesTrackModel;
class IPodPlaylistModel;


class IPodFeature : public LibraryFeature {
 Q_OBJECT
 public:
    IPodFeature(QObject* parent, TrackCollection* pTrackCollection);
    virtual ~IPodFeature();
    static bool isSupported();

    QVariant title();
    QIcon getIcon();

    bool dropAccept(QUrl url);
    bool dropAcceptChild(const QModelIndex& index, QUrl url);
    bool dragMoveAccept(QUrl url);
    bool dragMoveAcceptChild(const QModelIndex& index, QUrl url);

    TreeItemModel* getChildModel();

  public slots:
    void activate();
    void activate(bool forceReload);
    void activateChild(const QModelIndex& index);
    void onRightClick(const QPoint& globalPos);
    void onRightClickChild(const QPoint& globalPos, QModelIndex index);
    void onLazyChildExpandation(const QModelIndex& index);
    void onTrackCollectionLoaded();
    void slotAddToAutoDJ();
    void slotAddToAutoDJTop();
    void slotImportAsMixxxPlaylist();

  private:
    static QString getiTunesMusicPath();
    //returns the invisible rootItem for the sidebar model
    TreeItem* importLibrary();
    void parseTracks(QXmlStreamReader &xml);
    void parseTrack(QXmlStreamReader &xml, QSqlQuery &query);
    TreeItem* parsePlaylists(QXmlStreamReader &xml);
    void parsePlaylist(QXmlStreamReader &xml, QSqlQuery &query1, QSqlQuery &query2, TreeItem*);
    void clearTable(QString table_name);
    bool readNextStartElement(QXmlStreamReader& xml);
    void addToAutoDJ(bool bTop);

    QAction* m_pAddToAutoDJAction;
    QAction* m_pAddToAutoDJTopAction;
    QAction* m_pImportAsMixxxPlaylistAction;

	QModelIndex m_lastRightClickedIndex;

    ITunesTrackModel* m_pITunesTrackModel;
    IPodPlaylistModel* m_pIPodPlaylistModel;
    TreeItemModel m_childModel;
    QStringList m_playlists;
    TrackCollection* m_pTrackCollection;
    //a new DB connection for the worker thread
    QSqlDatabase m_database;
    bool m_isActivated;
    QString m_dbfile;

    QFutureWatcher<TreeItem*> m_future_watcher;
    QFuture<TreeItem*> m_future;
    QString m_title;
    bool m_cancelImport;

    QString m_dbItunesRoot;
    QString m_mixxxItunesRoot;

    Itdb_iTunesDB    *m_itdb;

//    static const QString ITDB_PATH_KEY;
};

#endif /* ITUNESFEATURE_H */
