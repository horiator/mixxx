
#ifndef CLEMENTINEFEATURE_H
#define CLEMENTINEFEATURE_H

#include <QStringListModel>
#include <QtSql>
#include <QFuture>
#include <QtConcurrentRun>
#include <QFutureWatcher>

#include "library/libraryfeature.h"
#include "library/trackcollection.h"
#include "library/treeitemmodel.h"
#include "library/treeitem.h"
// #include "library/banshee/bansheedbconnection.h"

#include "core/database.h"
#include "core/backgroundthread.h"


// class BansheePlaylistModel;

class ClementineFeature : public LibraryFeature {
 Q_OBJECT
 public:
    ClementineFeature(QObject* parent, TrackCollection* pTrackCollection, ConfigObject<ConfigValue>* pConfig);
    virtual ~ClementineFeature();
    static bool isSupported();
    static void prepareDbPath(ConfigObject<ConfigValue>* pConfig);

    QVariant title();
    QIcon getIcon();

    bool dropAccept(QUrl url);
    bool dropAcceptChild(const QModelIndex& index, QUrl url);
    bool dragMoveAccept(QUrl url);
    bool dragMoveAcceptChild(const QModelIndex& index, QUrl url);

    TreeItemModel* getChildModel();

  public slots:
    void activate();
    void activateChild(const QModelIndex& index);
    void onRightClick(const QPoint& globalPos);
    void onRightClickChild(const QPoint& globalPos, QModelIndex index);
    void onLazyChildExpandation(const QModelIndex& index);
    void slotAddToAutoDJ();
    void slotAddToAutoDJTop();
    void slotImportAsMixxxPlaylist();

  private:
    static QString getiTunesMusicPath();
    //returns the invisible rootItem for the sidebar model
    void addToAutoDJ(bool bTop);

    QAction* m_pAddToAutoDJAction;
    QAction* m_pAddToAutoDJTopAction;
    QAction* m_pImportAsMixxxPlaylistAction;

    QModelIndex m_lastRightClickedIndex;

//    BansheePlaylistModel* m_pBansheePlaylistModel;
    TreeItemModel m_childModel;
    QStringList m_playlists;
    TrackCollection* m_pTrackCollection;
    //a new DB connection for the worker thread

//    BansheeDbConnection m_connection;

    BackgroundThread<Database>* m_pClementineDatabaseThread;

    QSqlDatabase m_database;
    bool m_isActivated;
    QString m_dbfile;

    QFutureWatcher<TreeItem*> m_future_watcher;
    QFuture<TreeItem*> m_future;
    QString m_title;
    bool m_cancelImport;

    static QString m_databaseFile;

    QString m_mixxxItunesRoot;

    static const QString CLEMENTINE_MOUNT_KEY;
};

#endif /* CLEMENTINEFEATURE_H */
