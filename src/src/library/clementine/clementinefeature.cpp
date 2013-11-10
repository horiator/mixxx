#include <QMessageBox>
#include <QtDebug>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QList>


// #include "library/clementine/bansheedbconnection.h"
#include "clementinefeature.h"
#include "clementineview.h"
#include "widget/wlibrary.h"
// #include "library/banshee/bansheeplaylistmodel.h"
#include "library/dao/settingsdao.h"

// from clementine-player
#include "library/library.h"
#include "core/taskmanager.h"
#include "core/tagreaderclient.h"
#include "core/application.h"




const QString ClementineFeature::CLEMENTINE_MOUNT_KEY = "mixxx.ClementineFeature.mount";
const QString ClementineFeature::m_sClementineViewName = QString("Clementine");

QString ClementineFeature::m_databaseFile;

ClementineFeature::ClementineFeature(QObject* parent, TrackCollection* pTrackCollection, ConfigObject<ConfigValue>* pConfig)
        : LibraryFeature(parent),
          m_pTrackCollection(pTrackCollection),
          m_cancelImport(false),
          m_pClementineApp(NULL),
          m_view(NULL),
          m_pConfig(pConfig)
{
    //    m_pBansheePlaylistModel = new BansheePlaylistModel(this, m_pTrackCollection, &m_connection);
    m_isActivated = false;
    m_title = tr("Clementine");

    m_pAddToAutoDJAction = new QAction(tr("Add to Auto DJ bottom"),this);
    connect(m_pAddToAutoDJAction, SIGNAL(triggered()),
            this, SLOT(slotAddToAutoDJ()));

    m_pAddToAutoDJTopAction = new QAction(tr("Add to Auto DJ top 2"),this);
    connect(m_pAddToAutoDJTopAction, SIGNAL(triggered()),
            this, SLOT(slotAddToAutoDJTop()));

    m_pImportAsMixxxPlaylistAction = new QAction(tr("Import as Mixxx Playlist"), this);
    connect(m_pImportAsMixxxPlaylistAction, SIGNAL(triggered()),
            this, SLOT(slotImportAsMixxxPlaylist()));
}

ClementineFeature::~ClementineFeature() {

    if (m_pClementineApp) {
        delete m_pClementineApp;
    }
//    if (m_view) {
//        delete m_view;
//    }

    qDebug() << "~ClementineFeature()";
    // stop import thread, if still running
    m_cancelImport = true;
    if (m_future.isRunning()) {
        qDebug() << "m_future still running";
        m_future.waitForFinished();
        qDebug() << "m_future finished";
    }

//    delete m_pBansheePlaylistModel;
    delete m_pAddToAutoDJAction;
    delete m_pAddToAutoDJTopAction;
    delete m_pImportAsMixxxPlaylistAction;
}

// static
bool ClementineFeature::isSupported() {
    return !m_databaseFile.isEmpty();
}

// static
void ClementineFeature::prepareDbPath(ConfigObject<ConfigValue>* pConfig) {
    m_databaseFile = pConfig->getValueString(ConfigKey("[Banshee]","Database"));
    if (!QFile::exists(m_databaseFile)) {
        // Fall back to default
//        m_databaseFile = BansheeDbConnection::getDatabaseFile();
    }
}

QVariant ClementineFeature::title() {
    return m_title;
}

QIcon ClementineFeature::getIcon() {
    return QIcon(":/images/library/ic_library_clementine.png");
}

void ClementineFeature::activate() {
    //qDebug("ClementineFeature::activate()");

    if (!m_isActivated) {

        m_pClementineApp = new Application(Application::MIXXX_MODE, this);

        // setup connections
        connect(m_pClementineApp->database(), SIGNAL(Error(QString)), SLOT(showErrorDialog(QString)));
        connect(m_pClementineApp, SIGNAL(ErrorAdded(QString)), SLOT(showErrorDialog(QString)));
        //connect(app, SIGNAL(SettingsDialogRequested(SettingsDialog::Page)),
        //        SLOT(OpenSettingsDialogAtPage(SettingsDialog::Page)));

        int db_version = m_pClementineApp->database()->startup_schema_version();
        int feature_version = m_pClementineApp->database()->current_schema_version();

        if (db_version != feature_version) {
            QMessageBox::warning(
                    NULL,
                    tr("Error Loading Clementine database"),
                    tr("There was an error loading your Clementine database\n") +
                    QString(tr("Version mismatch: Clementine Database version: %1 Mixxx version: %2\n")).arg(
                            QString::number(db_version), QString::number(feature_version)));
            return;
        }

        qDebug() << "Using Clementine Database Schema V" << db_version;


        m_view->setApplication(m_pClementineApp);


        qDebug() << "ClementineFeature::importLibrary() ";

/*
        TreeItem* playlist_root = new TreeItem();

        QList<struct BansheeDbConnection::Playlist> list = m_connection.getPlaylists();

        struct BansheeDbConnection::Playlist playlist;
        foreach (playlist, list) {
            qDebug() << playlist.name;
            // append the playlist to the child model
            TreeItem *item = new TreeItem(playlist.name, playlist.playlistId, this, playlist_root);
            playlist_root->appendChild(item);
        };

        if(playlist_root){
            m_childModel.setRootItem(playlist_root);
            if (m_isActivated) {
                activate();
            }
            qDebug() << "Banshee library loaded: success";
        }
*/

        m_isActivated =  true;

        //calls a slot in the sidebarmodel such that 'isLoading' is removed from the feature title.
        m_title = tr("Clementine");
        emit(featureLoadingFinished(this));
    }

    emit(switchToView(m_sClementineViewName));

//    m_pBansheePlaylistModel->setPlaylist(0); // Gets the master playlist
//    emit(showTrackModel(m_pBansheePlaylistModel));
}

void ClementineFeature::bindWidget(WLibrarySidebar* /*sidebarWidget*/,
                               WLibrary* libraryWidget,
                               MixxxKeyboard* keyboard) {

    m_view = new ClementineView((QWidget*)libraryWidget, m_pConfig, m_pTrackCollection);

    //m_view->view()->setModel(library_sort_model_);
    //m_view->view()->SetLibrary(library_->model());
    //m_view->view()->SetTaskManager(task_manager_);
    //m_view->view()->SetDeviceManager(devices_);
    //m_view->view()->SetCoverProviders(cover_providers_);

    // m_pAutoDJView->installEventFilter(keyboard);

    libraryWidget->registerView(m_sClementineViewName, m_view);

    connect(m_view, SIGNAL(loadTrack(TrackPointer)),
            this, SIGNAL(loadTrack(TrackPointer)));
    connect(m_view, SIGNAL(loadTrackToPlayer(TrackPointer, QString, bool)),
            this, SIGNAL(loadTrackToPlayer(TrackPointer, QString, bool)));
}

void ClementineFeature::activateChild(const QModelIndex& index) {
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    //qDebug() << "BansheeFeature::activateChild " << item->data() << " " << item->dataPath();
    QString playlist = item->dataPath().toString();
    int playlistID = playlist.toInt();
    if (playlistID > 0) {
        qDebug() << "Activating " << item->data().toString();
//        m_pBansheePlaylistModel->setPlaylist(playlistID);
//        emit(showTrackModel(m_pBansheePlaylistModel));
    }
}

TreeItemModel* ClementineFeature::getChildModel() {
    return &m_childModel;
}

void ClementineFeature::onRightClick(const QPoint& globalPos) {
    Q_UNUSED(globalPos);
}

void ClementineFeature::onRightClickChild(const QPoint& globalPos, QModelIndex index) {
    //Save the model index so we can get it in the action slots...
    m_lastRightClickedIndex = index;

    //Create the right-click menu
    QMenu menu(NULL);
    menu.addAction(m_pAddToAutoDJAction);
    menu.addAction(m_pAddToAutoDJTopAction);
    menu.addSeparator();
    menu.addAction(m_pImportAsMixxxPlaylistAction);
    menu.exec(globalPos);
}

bool ClementineFeature::dropAccept(QUrl url) {
    Q_UNUSED(url);
    return false;
}

bool ClementineFeature::dropAcceptChild(const QModelIndex& index, QUrl url) {
    Q_UNUSED(index);
    Q_UNUSED(url);
    return false;
}

bool ClementineFeature::dragMoveAccept(QUrl url) {
    Q_UNUSED(url);
    return false;
}

bool ClementineFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
    Q_UNUSED(index);
    Q_UNUSED(url);
    return false;
}


void ClementineFeature::onLazyChildExpandation(const QModelIndex &index){
    Q_UNUSED(index);
    //Nothing to do because the childmodel is not of lazy nature.
}

void ClementineFeature::slotAddToAutoDJ() {
    //qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();
    addToAutoDJ(false); // Top = True
}


void ClementineFeature::slotAddToAutoDJTop() {
    //qDebug() << "slotAddToAutoDJTop() row:" << m_lastRightClickedIndex.data();
    addToAutoDJ(true); // bTop = True
}

void ClementineFeature::addToAutoDJ(bool bTop) {

    PlaylistDAO& playlistDao = m_pTrackCollection->getPlaylistDAO();
    int iAutoDJPlaylistId = playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);

    if (iAutoDJPlaylistId == -1) {
        return;
    }

    if (m_pData->hasUrls()) {
        foreach (QUrl url, m_pData->urls()) {
            qDebug() << "-----" << url;
            if (bTop) {
                // Load track to position two because position one is already loaded to the player
//                playlistDao.insertTrackIntoPlaylist(url, iAutoDJPlaylistId, 2);
            }
            else {
//                playlistDao.appendTrackToPlaylist(url, iAutoDJPlaylistId);
            }
        }
    }
        /*
     *
     *



        QModelIndexList indices = selectionModel()->selectedRows();

        TrackModel* trackModel = getTrackModel();
        foreach (QModelIndex index, indices) {
            TrackPointer pTrack;
            if (trackModel &&
                (pTrack = trackModel->getTrack(index))) {
                int iTrackId = pTrack->getId();
                if (iTrackId != -1) {
                    if (bTop) {
                        // Load track to position two because position one is already loaded to the player
                        playlistDao.insertTrackIntoPlaylist(iTrackId, iAutoDJPlaylistId, 2);
                    }
                    else {
                        playlistDao.appendTrackToPlaylist(iTrackId, iAutoDJPlaylistId);
                    }
                }
            }
        }

    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(m_lastRightClickedIndex.internalPointer());
        qDebug() << "ClementineFeature::addToAutoDJ " << item->data() << " " << item->dataPath();
        QString playlist = item->dataPath().toString();
        int playlistID = playlist.toInt();
        if (playlistID > 0) {
           BansheePlaylistModel* pPlaylistModelToAdd = new BansheePlaylistModel(this, m_pTrackCollection, &m_connection);
            pPlaylistModelToAdd->setPlaylist(playlistID);
            PlaylistDAO &playlistDao = m_pTrackCollection->getPlaylistDAO();
            int autoDJId = playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);

            int rows = pPlaylistModelToAdd->rowCount();
            for(int i = 0; i < rows; ++i){
                QModelIndex index = pPlaylistModelToAdd->index(i,0);
                if (index.isValid()) {
                    TrackPointer track = pPlaylistModelToAdd->getTrack(index);
                    if (bTop) {
                        // Start at position 2 because position 1 was already loaded to the deck
                        playlistDao.insertTrackIntoPlaylist(track->getId(), autoDJId, i+2);
                    } else {
                        playlistDao.appendTrackToPlaylist(track->getId(), autoDJId);
                    }
                }
            }
            delete pPlaylistModelToAdd;
            }
    }
    */
}

void ClementineFeature::slotImportAsMixxxPlaylist() {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(m_lastRightClickedIndex.internalPointer());
        QString playlistName = item->data().toString();
        QString playlistStId = item->dataPath().toString();
        int playlistID = playlistStId.toInt();
        qDebug() << "BansheeFeature::slotImportAsMixxxPlaylist " << playlistName << " " << playlistStId;
        if (playlistID > 0) {
/*            BansheePlaylistModel* pPlaylistModelToAdd = new BansheePlaylistModel(this, m_pTrackCollection, &m_connection);
            pPlaylistModelToAdd->setPlaylist(playlistID);
            PlaylistDAO &playlistDao = m_pTrackCollection->getPlaylistDAO();

            int playlistId = playlistDao.getPlaylistIdFromName(playlistName);
            int i = 1;

            if (playlistId != -1) {
                // Calculate a unique name
                playlistName += "(%1)";
                while (playlistId != -1) {
                    i++;
                    playlistId = playlistDao.getPlaylistIdFromName(playlistName.arg(i));
                }
                playlistName = playlistName.arg(i);
            }
            playlistId = playlistDao.createPlaylist(playlistName);

            if (playlistId != -1) {
                // Copy Tracks
                int rows = pPlaylistModelToAdd->rowCount();
                for (int i = 0; i < rows; ++i) {
                    QModelIndex index = pPlaylistModelToAdd->index(i,0);
                    if (index.isValid()) {
                        //qDebug() << pPlaylistModelToAdd->getTrackLocation(index);
                        TrackPointer track = pPlaylistModelToAdd->getTrack(index);
                        playlistDao.appendTrackToPlaylist(track->getId(), playlistId);
                    }
                }
            }
            else {
                QMessageBox::warning(NULL,
                                     tr("Playlist Creation Failed"),
                                     tr("An unknown error occurred while creating playlist: ")
                                      + playlistName);
            }

            delete pPlaylistModelToAdd;
            */
        }
    }
}

void ClementineFeature::showErrorDialog(const QString& message) {
    if (!m_error_dialog) {
        m_error_dialog = new ErrorDialog();
    }
    m_error_dialog->ShowMessage(message);
}

void ClementineFeature::slotLibraryViewRightClicked(QMimeData* data) {
    // setup Context Menu actions
    qDebug() << "slotLibraryViewRightClicked";

    m_pData = data;

}
/*
TrackPointer BansheePlaylistModel::getTrack(const QModelIndex& index) const {

    QString location;

    location = getTrackLocation(index);

    if (location.isEmpty()) {
        return TrackPointer();
    }

    int row = index.row();

    TrackDAO& track_dao = m_pTrackCollection->getTrackDAO();
    int track_id = track_dao.getTrackId(location);
    bool track_already_in_library = track_id >= 0;
    if (track_id < 0) {
        // Add Track to library
        track_id = track_dao.addTrack(location, true);
    }

    TrackPointer pTrack;

    if (track_id < 0) {
        // Add Track to library failed, create a transient TrackInfoObject
        pTrack = TrackPointer(new TrackInfoObject(location), &QObject::deleteLater);
    } else {
        pTrack = track_dao.getTrack(track_id);
    }

    // If this track was not in the Mixxx library it is now added and will be
    // saved with the metadata from iTunes. If it was already in the library
    // then we do not touch it so that we do not over-write the user's metadata.
    if (!track_already_in_library) {
        pTrack->setArtist(m_sortedPlaylist.at(row).pArtist->name);
        pTrack->setTitle(m_sortedPlaylist.at(row).pTrack->title);
        pTrack->setDuration(m_sortedPlaylist.at(row).pTrack->duration);
        pTrack->setAlbum(m_sortedPlaylist.at(row).pAlbum->title);
        pTrack->setYear(QString::number(m_sortedPlaylist.at(row).pTrack->year));
        pTrack->setRating(m_sortedPlaylist.at(row).pTrack->rating);
        pTrack->setGenre(m_sortedPlaylist.at(row).pTrack->genre);
        pTrack->setTrackNumber(QString::number(m_sortedPlaylist.at(row).pTrack->tracknumber));
        float bpm = ((float)m_sortedPlaylist.at(row).pTrack->bpm)/10.0;
        pTrack->setBpm(bpm);
        pTrack->setBitrate(m_sortedPlaylist.at(row).pTrack->bitrate);
        pTrack->setComment(m_sortedPlaylist.at(row).pTrack->comment);
        pTrack->setComposer(m_sortedPlaylist.at(row).pTrack->composer);
        // If the track has a BPM, then give it a static beatgrid.
        if (bpm > 0) {
            BeatsPointer pBeats = BeatFactory::makeBeatGrid(pTrack, bpm, 0);
            pTrack->setBeats(pBeats);
        }
    }
    return pTrack;
}
*/
