#include <QMessageBox>
#include <QtDebug>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QAction>

#include "library/ipod/ipodfeature.h"

#include "library/itunes/itunestrackmodel.h"
#include "library/ipod/ipodplaylistmodel.h"
#include "library/dao/settingsdao.h"

extern "C" {
#include <glib-object.h> // g_type_init
}

// const QString IPodFeature::ITDB_PATH_KEY = "mixxx.IPodFeature.itdbpath";


IPodFeature::IPodFeature(QObject* parent, TrackCollection* pTrackCollection)
        : LibraryFeature(parent),
          m_pTrackCollection(pTrackCollection),
          m_cancelImport(false),
          m_itdb( 0 )
{
    m_pIPodPlaylistModel = new IPodPlaylistModel(this, m_pTrackCollection);
    m_isActivated = false;
    m_title = tr("iPod");

    connect(&m_future_watcher, SIGNAL(finished()), this, SLOT(onTrackCollectionLoaded()));

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

IPodFeature::~IPodFeature() {
	qDebug() << "~IPodFeature()";
	// stop import thread, if still running
	m_cancelImport = true;
	if (m_future.isRunning()) {
		qDebug() << "m_future still running";
		m_future.waitForFinished();
		qDebug() << "m_future finished";
	}
    if (m_itdb) {
        itdb_free( m_itdb );
    }
    delete m_pIPodPlaylistModel;
    delete m_pAddToAutoDJAction;
    delete m_pAddToAutoDJTopAction;
    delete m_pImportAsMixxxPlaylistAction;
}

// static
bool IPodFeature::isSupported() {
    // itunes db might just be elsewhere, don't rely on it being in its
    // normal place. And since we will load an itdb on any platform...
    return true;
}


QVariant IPodFeature::title() {
    return m_title;
}

QIcon IPodFeature::getIcon() {
    return QIcon(":/images/library/ic_library_ipod.png");
}

void IPodFeature::activate() {
    activate(false);
}

void IPodFeature::activate(bool forceReload) {
    //qDebug("IPodFeature::activate()");
	GError *err = 0;

    if (!m_isActivated || forceReload) {

        SettingsDAO settings(m_pTrackCollection->getDatabase());
//        QString dbSetting(settings.getValue(ITDB_PATH_KEY));
        QString dbSetting;
        // if a path exists in the database, use it
        if (!dbSetting.isEmpty() && QFile::exists(dbSetting)) {
            m_dbfile = dbSetting;
        } else {
            // No Path in settings
            m_dbfile = "";
        }
        // if the path we got between the default and the database doesn't
        // exist, ask for a new one and use/save it if it exists
        if (!QFile::exists(m_dbfile)) {
            m_dbfile = QFileDialog::getExistingDirectory(
            	NULL,
                tr("Select your iPod mount"),
                "/media");
            if (m_dbfile.isEmpty()) {
                return;
            }
        }

        qDebug() << "Calling the libgpod db parser";
        m_itdb = itdb_parse( m_dbfile.toUtf8() ,  &err );

        if( err )
        {
            qDebug() << "There was an error, attempting to free db: " << err->message;
            QMessageBox::warning(
                NULL,
                tr("Error Loading iPod database"),
                err->message);
            g_error_free( err );
            if ( m_itdb )
            {
                itdb_free( m_itdb );
                m_itdb = 0;
            }
        } else {
        	// now we have an iPod with a valid itdb
        	m_isActivated =  true;

            QThreadPool::globalInstance()->setMaxThreadCount(4); //Tobias decided to use 4
            // Let a worker thread do the XML parsing
            m_future = QtConcurrent::run(this, &IPodFeature::importLibrary);
            m_future_watcher.setFuture(m_future);
            m_title = tr("(loading) iPod"); // (loading) at start in respect to small displays
            //calls a slot in the sidebar model such that 'iTunes (isLoading)' is displayed.
            emit (featureIsLoading(this));
        }
    }
    else{
        m_pIPodPlaylistModel->setPlaylist(itdb_playlist_mpl(m_itdb)); // Gets the master playlist
        emit(showTrackModel(m_pIPodPlaylistModel));
    }
}

void IPodFeature::activateChild(const QModelIndex& index) {
	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
	qDebug() << "IPodFeature::activateChild " << item->data() << " " << item->dataPath();
    QString playlist = item->dataPath().toString();
    Itdb_Playlist* pPlaylist = (Itdb_Playlist*)playlist.toUInt();

    if (pPlaylist) {
    	qDebug() << "Activating " << QString::fromUtf8(pPlaylist->name);
    }
    m_pIPodPlaylistModel->setPlaylist(pPlaylist);
    emit(showTrackModel(m_pIPodPlaylistModel));
}

TreeItemModel* IPodFeature::getChildModel() {
    return &m_childModel;
}

void IPodFeature::onRightClick(const QPoint& globalPos) {
    QMenu menu;
    QAction useDefault(tr("Use Default Library"), &menu);
    QAction chooseNew(tr("Choose Library..."), &menu);
    menu.addAction(&useDefault);
    menu.addAction(&chooseNew);
    QAction *chosen(menu.exec(globalPos));
    if (chosen == &useDefault) {
        SettingsDAO settings(m_database);
//        settings.setValue(ITDB_PATH_KEY, QString());
        activate(true); // clears tables before parsing
    } else if (chosen == &chooseNew) {
        SettingsDAO settings(m_database);
        QString dbfile = QFileDialog::getOpenFileName(NULL,
            tr("Select your iPod mount"),
            QDir::homePath(), "*.xml");
        if (dbfile.isEmpty() || !QFile::exists(dbfile)) {
            return;
        }
//        settings.setValue(ITDB_PATH_KEY, dbfile);
        activate(true); // clears tables before parsing
    }
	m_lastRightClickedIndex = QModelIndex();
}

void IPodFeature::onRightClickChild(const QPoint& globalPos, QModelIndex index) {
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

bool IPodFeature::dropAccept(QUrl url) {
	Q_UNUSED(url);
	return false;
}

bool IPodFeature::dropAcceptChild(const QModelIndex& index, QUrl url) {
	Q_UNUSED(index);
	Q_UNUSED(url);
    return false;
}

bool IPodFeature::dragMoveAccept(QUrl url) {
	Q_UNUSED(url);
    return false;
}

bool IPodFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
	Q_UNUSED(index);
	Q_UNUSED(url);
    return false;
}

/*
 * This method is executed in a separate thread
 * via QtConcurrent::run
 */
TreeItem* IPodFeature::importLibrary() {
    // Give thread a low priority
    QThread* thisThread = QThread::currentThread();
    thisThread->setPriority(QThread::LowestPriority);

    qDebug() << "IPodFeature::importLibrary() ";

    TreeItem* playlist_root = new TreeItem();

    if (m_itdb) {
		GList* playlist_node;

		for (playlist_node = g_list_first(m_itdb->playlists);
			 playlist_node != NULL;
			 playlist_node = g_list_next(playlist_node))
		{
			Itdb_Playlist* pPlaylist;
			pPlaylist = (Itdb_Playlist*)playlist_node->data;
			if (!itdb_playlist_is_mpl(pPlaylist)) {
				QString playlistname = QString::fromUtf8(pPlaylist->name);
				qDebug() << playlistname;
				// append the playlist to the child model
				TreeItem *item = new TreeItem(playlistname, QString::number((uint)pPlaylist), this, playlist_root);
				playlist_root->appendChild(item);
			}
		}
    }
    return playlist_root;
}

void IPodFeature::onTrackCollectionLoaded(){
    TreeItem* root = m_future.result();
    if(root){
        m_childModel.setRootItem(root);
        if (m_isActivated) {
        	activate();
        }
        qDebug() << "iPod library loaded: success";
    }
    else{
        QMessageBox::warning(
            NULL,
            tr("Error Loading iPod database"),
            tr("There was an error loading your iPod database. Some of "
               "your iPod tracks or playlists may not have loaded."));
    }
    //calls a slot in the sidebarmodel such that 'isLoading' is removed from the feature title.
    m_title = tr("iPod");
    emit(featureLoadingFinished(this));
    activate();
}

void IPodFeature::onLazyChildExpandation(const QModelIndex &index){
	Q_UNUSED(index);
	//Nothing to do because the childmodel is not of lazy nature.
}

void IPodFeature::slotAddToAutoDJ() {
    //qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();
	addToAutoDJ(false); // Top = True
}


void IPodFeature::slotAddToAutoDJTop() {
    //qDebug() << "slotAddToAutoDJTop() row:" << m_lastRightClickedIndex.data();
	addToAutoDJ(true); // bTop = True
}

void IPodFeature::addToAutoDJ(bool bTop) {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
    	TreeItem *item = static_cast<TreeItem*>(m_lastRightClickedIndex.internalPointer());
    	qDebug() << "IPodFeature::addToAutoDJ " << item->data() << " " << item->dataPath();
        QString playlist = item->dataPath().toString();
        Itdb_Playlist* pPlaylist = (Itdb_Playlist*)playlist.toUInt();
        if (pPlaylist) {
			IPodPlaylistModel* pPlaylistModelToAdd = new IPodPlaylistModel(this, m_pTrackCollection);
	    	pPlaylistModelToAdd->setPlaylist(pPlaylist);
			PlaylistDAO &playlistDao = m_pTrackCollection->getPlaylistDAO();
			int autoDJId = playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);

			int rows = pPlaylistModelToAdd->rowCount();
			for(int i = 0; i < rows; ++i){
				QModelIndex index = pPlaylistModelToAdd->index(i,0);
				if (index.isValid()) {
	//            	qDebug() << pPlaylistModelToAdd->getTrackLocation(index);
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
}

void IPodFeature::slotImportAsMixxxPlaylist() {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
    	TreeItem *item = static_cast<TreeItem*>(m_lastRightClickedIndex.internalPointer());
    	qDebug() << "IPodFeature::slotImportAsMixxxPlaylist " << item->data() << " " << item->dataPath();
		QString playlist = item->dataPath().toString();
		Itdb_Playlist* pPlaylist = (Itdb_Playlist*)playlist.toUInt();
		if (pPlaylist) {
			IPodPlaylistModel* pPlaylistModelToAdd = new IPodPlaylistModel(this, m_pTrackCollection);
			pPlaylistModelToAdd->setPlaylist(pPlaylist);
			PlaylistDAO &playlistDao = m_pTrackCollection->getPlaylistDAO();

			int playlistId = playlistDao.getPlaylistIdFromName(playlist);
			int i = 1;

			if (playlistId != -1) {
				// Calculate a unique name
				playlist += "(%1)";
				while (playlistId != -1) {
					i++;
					playlistId = playlistDao.getPlaylistIdFromName(playlist.arg(i));
				}
				playlist = playlist.arg(i);
			}
			playlistId = playlistDao.createPlaylist(playlist);

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
									  + playlist);
			}

			delete pPlaylistModelToAdd;
		}
    }
    emit(featureUpdated());
}
