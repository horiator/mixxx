#include <QMessageBox>
#include <QtDebug>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QAction>

#include "library/itunes/itunesfeature.h"

#include "library/basetrackcache.h"
#include "library/dao/settingsdao.h"
#include "library/itunes/itunesplaylistmodel.h"
#include "library/itunes/itunestrackmodel.h"
#include "library/queryutil.h"

const QString ITunesFeature::ITDB_PATH_KEY = "mixxx.itunesfeature.itdbpath";


ITunesFeature::ITunesFeature(QObject* parent, TrackCollection* pTrackCollection)
        : LibraryFeature(parent),
          m_pTrackCollection(pTrackCollection),
          m_cancelImport(false) {
    QString tableName = "itunes_library";
    QString idColumn = "id";
    QStringList columns;
    columns << "id"
            << "artist"
            << "title"
            << "album"
            << "year"
            << "genre"
            << "tracknumber"
            << "location"
            << "comment"
            << "duration"
            << "bitrate"
            << "bpm"
            << "rating";
    pTrackCollection->addTrackSource(
        QString("itunes"), QSharedPointer<BaseTrackCache>(
            new BaseTrackCache(m_pTrackCollection, tableName, idColumn,
                               columns, false)));
    m_pITunesTrackModel = new ITunesTrackModel(this, m_pTrackCollection);
    m_pITunesPlaylistModel = new ITunesPlaylistModel(this, m_pTrackCollection);
    m_isActivated = false;
    m_title = tr("iTunes");

    m_database = QSqlDatabase::cloneDatabase( pTrackCollection->getDatabase(), "ITUNES_SCANNER");

    //Open the database connection in this thread.
    if (!m_database.open()) {
        qDebug() << "Failed to open database for iTunes scanner." << m_database.lastError();
    }
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

ITunesFeature::~ITunesFeature() {
    m_database.close();
    m_cancelImport = true;
    m_future.waitForFinished();
    delete m_pITunesTrackModel;
    delete m_pITunesPlaylistModel;
    delete m_pAddToAutoDJAction;
    delete m_pAddToAutoDJTopAction;
    delete m_pImportAsMixxxPlaylistAction;
}

// static
bool ITunesFeature::isSupported() {
    // itunes db might just be elsewhere, don't rely on it being in its
    // normal place. And since we will load an itdb on any platform...
    return true;
}


QVariant ITunesFeature::title() {
    return m_title;
}

QIcon ITunesFeature::getIcon() {
    return QIcon(":/images/library/ic_library_itunes.png");
}

void ITunesFeature::activate() {
    activate(false);
}

void ITunesFeature::activate(bool forceReload) {
    //qDebug("ITunesFeature::activate()");

    if (!m_isActivated || forceReload) {

        SettingsDAO settings(m_pTrackCollection->getDatabase());
        QString dbSetting(settings.getValue(ITDB_PATH_KEY));
        // if a path exists in the database, use it
        if (!dbSetting.isEmpty() && QFile::exists(dbSetting)) {
            m_dbfile = dbSetting;
        } else {
            // No Path in settings, try the default
            m_dbfile = getiTunesMusicPath();
        }
        // if the path we got between the default and the database doesn't
        // exist, ask for a new one and use/save it if it exists
        if (!QFile::exists(m_dbfile)) {
            m_dbfile = QFileDialog::getOpenFileName(NULL,
                tr("Select your iTunes library"),
                QDir::homePath(), "*.xml");
            if (m_dbfile.isEmpty() || !QFile::exists(m_dbfile)) {
                return;
            }
            settings.setValue(ITDB_PATH_KEY, m_dbfile);
        }
        m_isActivated =  true;
        /* Ususally the maximum number of threads
         * is > 2 depending on the CPU cores
         * Unfortunately, within VirtualBox
         * the maximum number of allowed threads
         * is 1 at all times We'll need to increase
         * the number to > 1, otherwise importing the music collection
         * takes place when the GUI threads terminates, i.e., on
         * Mixxx shutdown.
         */
        QThreadPool::globalInstance()->setMaxThreadCount(4); //Tobias decided to use 4
        // Let a worker thread do the XML parsing
        m_future = QtConcurrent::run(this, &ITunesFeature::importLibrary);
        m_future_watcher.setFuture(m_future);
        m_title = tr("(loading) iTunes");
        //calls a slot in the sidebar model such that 'iTunes (isLoading)' is displayed.
        emit (featureIsLoading(this));
    }
    else{
        emit(showTrackModel(m_pITunesTrackModel));
    }

}

void ITunesFeature::activateChild(const QModelIndex& index) {
    //qDebug() << "ITunesFeature::activateChild()" << index;
    QString playlist = index.data().toString();
    qDebug() << "Activating " << playlist;
    m_pITunesPlaylistModel->setPlaylist(playlist);
    emit(showTrackModel(m_pITunesPlaylistModel));
}

TreeItemModel* ITunesFeature::getChildModel() {
    return &m_childModel;
}

void ITunesFeature::onRightClick(const QPoint& globalPos) {
    QMenu menu;
    QAction useDefault(tr("Use Default Library"), &menu);
    QAction chooseNew(tr("Choose Library..."), &menu);
    menu.addAction(&useDefault);
    menu.addAction(&chooseNew);
    QAction *chosen(menu.exec(globalPos));
    if (chosen == &useDefault) {
        SettingsDAO settings(m_database);
        settings.setValue(ITDB_PATH_KEY, QString());
        activate(true); // clears tables before parsing
    } else if (chosen == &chooseNew) {
        SettingsDAO settings(m_database);
        QString dbfile = QFileDialog::getOpenFileName(NULL,
            tr("Select your iTunes library"),
            QDir::homePath(), "*.xml");
        if (dbfile.isEmpty() || !QFile::exists(dbfile)) {
            return;
        }
        settings.setValue(ITDB_PATH_KEY, dbfile);
        activate(true); // clears tables before parsing
    }
	m_lastRightClickedIndex = QModelIndex();
}

void ITunesFeature::onRightClickChild(const QPoint& globalPos, QModelIndex index) {
    //Save the model index so we can get it in the action slots...
    m_lastRightClickedIndex = index;

    //Create the right-click menu
    QMenu menu;
    menu.addAction(m_pAddToAutoDJAction);
    menu.addAction(m_pAddToAutoDJTopAction);
    menu.addSeparator();
    menu.addAction(m_pImportAsMixxxPlaylistAction);
    menu.exec(globalPos);
}

bool ITunesFeature::dropAccept(QUrl url) {
    Q_UNUSED(url);
    return false;
}

bool ITunesFeature::dropAcceptChild(const QModelIndex& index, QUrl url) {
    Q_UNUSED(index);
    Q_UNUSED(url);
    return false;
}

bool ITunesFeature::dragMoveAccept(QUrl url) {
    Q_UNUSED(url);
    return false;
}

bool ITunesFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
    Q_UNUSED(index);
    Q_UNUSED(url);
    return false;
}

QString ITunesFeature::getiTunesMusicPath() {
    QString musicFolder;
#if defined(__APPLE__)
    musicFolder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation) + "/iTunes/iTunes Music Library.xml";
#elif defined(__WINDOWS__)
    musicFolder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation) + "\\iTunes\\iTunes Music Library.xml";
#else
		musicFolder = "";
#endif
    qDebug() << "ITunesLibrary=[" << musicFolder << "]";
    return musicFolder;
}
/*
 * This method is executed in a separate thread
 * via QtConcurrent::run
 */
TreeItem* ITunesFeature::importLibrary() {
    //Give thread a low priority
    QThread* thisThread = QThread::currentThread();
    thisThread->setPriority(QThread::LowestPriority);

    //Delete all table entries of iTunes feature
    ScopedTransaction transaction(m_database);
    clearTable("itunes_playlist_tracks");
    clearTable("itunes_library");
    clearTable("itunes_playlists");
    transaction.commit();

    qDebug() << "ITunesFeature::importLibrary() ";

    transaction.transaction();

    //Parse iTunes XML file using SAX (for performance)
    QFile itunes_file(m_dbfile);
    if (!itunes_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open iTunes music collection";
        return false;
    }
    QXmlStreamReader xml(&itunes_file);
    TreeItem* playlist_root = NULL;
    while (!xml.atEnd() && !m_cancelImport) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "key") {
            	QString key = xml.readElementText();
            	if (key == "Music Folder") {
					if (readNextStartElement(xml)) {
						// Normally the Folder Layout it some thing like that
						// iTunes/
						// iTunes/Album Artwork
						// iTunes/iTunes Media <- this is the "Music Folder"
						// iTunes/iTunes Music Library.xml <- this location we already knew
	                    QByteArray strlocbytes = xml.readElementText().toUtf8();
	                    QString music_folder = QUrl::fromEncoded(strlocbytes).toLocalFile();
						qDebug() << music_folder;
						int i = music_folder.lastIndexOf(QDir::separator(),-2); // Skip tailing separator if any
						if (i > -1) {
							// strip folder "iTunes Media" (at least iTunes 9 it)
							// or "iTunes Music" (at least iTunes 7 has it)
							m_dbItunesRoot = music_folder.left(i);
						}
						i = m_dbfile.lastIndexOf(QDir::separator());
						if (i > -1) {
							// folder "iTunes Media" in Path
							m_mixxxItunesRoot = m_dbfile.left(i);
			            }
                        // Remove matching tail part
						while (m_mixxxItunesRoot.right(1) == m_dbItunesRoot.right(1)) {
    						m_mixxxItunesRoot.chop(1);
    						m_dbItunesRoot.chop(1);
    					}
    				}
    			} else if (key == "Tracks") {
                	parseTracks(xml);
                    playlist_root = parsePlaylists(xml);
                }
            }
        }
    }

    itunes_file.close();

    // Even if an error occured, commit the transaction. The file may have been
    // half-parsed.
    transaction.commit();

    if (xml.hasError()) {
        // do error handling
        qDebug() << "Cannot process iTunes music collection";
        qDebug() << "XML ERROR: " << xml.errorString();
        if(playlist_root)
            delete playlist_root;
        playlist_root = NULL;
        return false;
    }
    return playlist_root;
}

void ITunesFeature::parseTracks(QXmlStreamReader &xml) {
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO itunes_library (id, artist, title, album, year, genre, comment, tracknumber,"
                  "bpm, bitrate,"
                  "duration, location,"
                  "rating ) "
                  "VALUES (:id, :artist, :title, :album, :year, :genre, :comment, :tracknumber,"
                  ":bpm, :bitrate,"
                  ":duration, :location," ":rating )");


    bool in_container_dictionary = false;
    bool in_track_dictionary = false;

    qDebug() << "Parse iTunes music collection";

    //read all sunsequent <dict> until we reach the closing ENTRY tag
    while (!xml.atEnd() && !m_cancelImport) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "dict") {
                if (!in_track_dictionary && !in_container_dictionary) {
                    in_container_dictionary = true;
                    continue;
                } else if (in_container_dictionary && !in_track_dictionary) {
                    //We are in a <dict> tag that holds track information
                    in_track_dictionary = true;
                    //Parse track here
                    parseTrack(xml, query);
                }
            }
        }

        if (xml.isEndElement() && xml.name() == "dict") {
            if (in_track_dictionary && in_container_dictionary) {
                in_track_dictionary = false;
                continue;
            } else if (in_container_dictionary && !in_track_dictionary) {
                // Done parsing tracks.
                in_container_dictionary = false;
                break;
            }
        }
    }
}

void ITunesFeature::parseTrack(QXmlStreamReader &xml, QSqlQuery &query) {
    //qDebug() << "----------------TRACK-----------------";
    int id = -1;
    QString title;
    QString artist;
    QString album;
    QString year;
    QString genre;
    QString location;

    int bpm = 0;
    int bitrate = 0;

    //duration of a track
    int playtime = 0;
    int rating = 0;
    QString comment;
    QString tracknumber;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "key") {
                QString key = xml.readElementText();
                QString content =  "";

                if (readNextStartElement(xml)) {
                    content = xml.readElementText();
                }

                //qDebug() << "Key: " << key << " Content: " << content;

                if (key == "Track ID") {
                    id = content.toInt();
                    continue;
                }
                if (key == "Name") {
                    title = content;
                    continue;
                }
                if (key == "Artist") {
                    artist = content;
                    continue;
                }
                if (key == "Album") {
                    album = content;
                    continue;
                }
                if (key == "Genre") {
                    genre = content;
                    continue;
                }
                if (key == "BPM") {
                    bpm = content.toInt();
                    continue;
                }
                if (key == "Bit Rate") {
                    bitrate =  content.toInt();
                    continue;
                }
                if (key == "Comments") {
                    comment = content;
                    continue;
                }
                if (key == "Total Time") {
                    playtime = (content.toInt() / 1000);
                    continue;
                }
                if (key == "Year") {
                    year = content;
                    continue;
                }
                if (key == "Location") {
                    QByteArray strlocbytes = content.toUtf8();
                    location = QUrl::fromEncoded(strlocbytes).toLocalFile();
                    // Replace first part of location with the mixxx iTunes Root
                    // on systems where iTunes installed it only strips //localhost
                    // on iTunes from foreign systems the mount point is replaced
                    if (!m_dbItunesRoot.isEmpty()) {
                    	location.replace( m_dbItunesRoot, m_mixxxItunesRoot);
                    }
                    continue;
                }
                if (key == "Track Number") {
                    tracknumber = content;
                    continue;
                }
                if (key == "Rating") {
                    //value is an integer and ranges from 0 to 100
                    rating = (content.toInt() / 20);
                    continue;
                }
            }
        }
        //exit loop on closing </dict>
        if (xml.isEndElement() && xml.name() == "dict") {
            break;
        }
    }
    /* If we reach the end of <dict>
     * Save parsed track to database
     */
    query.bindValue(":id", id);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":album", album);
    query.bindValue(":genre", genre);
    query.bindValue(":year", year);
    query.bindValue(":duration", playtime);
    query.bindValue(":location", location);
    query.bindValue(":rating", rating);
    query.bindValue(":comment", comment);
    query.bindValue(":tracknumber", tracknumber);
    query.bindValue(":bpm", bpm);
    query.bindValue(":bitrate", bitrate);

    bool success = query.exec();

    if (!success) {
        qDebug() << "SQL Error in itunesfeature.cpp: line" << __LINE__ << " " << query.lastError();
        return;
    }
}

TreeItem* ITunesFeature::parsePlaylists(QXmlStreamReader &xml) {
    qDebug() << "Parse iTunes playlists";
    TreeItem* rootItem = new TreeItem();
    QSqlQuery query_insert_to_playlists(m_database);
    query_insert_to_playlists.prepare("INSERT INTO itunes_playlists (id, name) "
                                      "VALUES (:id, :name)");

    QSqlQuery query_insert_to_playlist_tracks(m_database);
    query_insert_to_playlist_tracks.prepare(
        "INSERT INTO itunes_playlist_tracks (playlist_id, track_id, position) "
        "VALUES (:playlist_id, :track_id, :position)");

    while (!xml.atEnd() && !m_cancelImport) {
        xml.readNext();
        //We process and iterate the <dict> tags holding playlist summary information here
        if (xml.isStartElement() && xml.name() == "dict") {
            parsePlaylist(xml,
                          query_insert_to_playlists,
                          query_insert_to_playlist_tracks,
                          rootItem);
            continue;
        }
        if (xml.isEndElement()) {
            if (xml.name() == "array")
                break;
        }
    }
    return rootItem;
}

bool ITunesFeature::readNextStartElement(QXmlStreamReader& xml) {
    QXmlStreamReader::TokenType token = QXmlStreamReader::NoToken;
    while (token != QXmlStreamReader::EndDocument && token != QXmlStreamReader::Invalid) {
        token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            return true;
        }
    }
    return false;
}

void ITunesFeature::parsePlaylist(QXmlStreamReader &xml, QSqlQuery &query_insert_to_playlists,
                                  QSqlQuery &query_insert_to_playlist_tracks, TreeItem* root) {
    //qDebug() << "Parse Playlist";

    QString playlistname;
    int playlist_id = -1;
    int playlist_position = -1;
    int track_reference = -1;
    //indicates that we haven't found the <
    bool isSystemPlaylist = false;

    QString key;


    //We process and iterate the <dict> tags holding playlist summary information here
    while (!xml.atEnd() && !m_cancelImport) {
        xml.readNext();

        if (xml.isStartElement()) {

            if (xml.name() == "key") {
                QString key = xml.readElementText();
                /*
                 * The rules are processed in sequence
                 * That is, XML is ordered.
                 * For iTunes Playlist names are always followed by the ID.
                 * Afterwars the playlist entries occur
                 */
                if (key == "Name") {
                    readNextStartElement(xml);
                    playlistname = xml.readElementText();
                    continue;
                }
                //When parsing the ID, the playlistname has already been found
                if (key == "Playlist ID") {
                    readNextStartElement(xml);
                    playlist_id = xml.readElementText().toInt();
                    playlist_position = 1;
                    continue;
                }
                //Hide playlists that are system playlists
                if (key == "Master" || key == "Movies" || key == "TV Shows" || key == "Music" ||
                   key == "Books" || key == "Purchased") {
                    isSystemPlaylist = true;
                    continue;
                }

                if (key == "Playlist Items") {
                    //if the playlist is prebuild don't hit the database
                    if (isSystemPlaylist) continue;
                    query_insert_to_playlists.bindValue(":id", playlist_id);
                    query_insert_to_playlists.bindValue(":name", playlistname);

                    bool success = query_insert_to_playlists.exec();
                    if (!success) {
                        qDebug() << "SQL Error in ITunesTableModel.cpp: line" << __LINE__
                                 << " " << query_insert_to_playlists.lastError();
                        return;
                    }
                    //append the playlist to the child model
                    TreeItem *item = new TreeItem(playlistname, playlistname, this, root);
                    root->appendChild(item);

                }
                /*
                 * When processing playlist entries, playlist name and id have already been processed and persisted
                 */
                if (key == "Track ID") {
                    track_reference = -1;

                    readNextStartElement(xml);
                    track_reference = xml.readElementText().toInt();

                    query_insert_to_playlist_tracks.bindValue(":playlist_id", playlist_id);
                    query_insert_to_playlist_tracks.bindValue(":track_id", track_reference);
                    query_insert_to_playlist_tracks.bindValue(":position", playlist_position++);

                    //Insert tracks if we are not in a pre-build playlist
                    if (!isSystemPlaylist && !query_insert_to_playlist_tracks.exec()) {
                        qDebug() << "SQL Error in ITunesFeature.cpp: line" << __LINE__ << " "
                                 << query_insert_to_playlist_tracks.lastError();
                        qDebug() << "trackid" << track_reference;
                        qDebug() << "playlistname; " << playlistname;
                        qDebug() << "-----------------";
                    }
                }
            }
        }
        if (xml.isEndElement()) {
            if (xml.name() == "array") {
                //qDebug() << "exit playlist";
                break;
            }
        }
    }
}

void ITunesFeature::clearTable(QString table_name) {
    QSqlQuery query(m_database);
    query.prepare("delete from "+table_name);
    bool success = query.exec();

    if (!success) {
        qDebug() << "Could not delete remove old entries from table "
                 << table_name << " : " << query.lastError();
    } else {
        qDebug() << "iTunes table entries of '"
                 << table_name <<"' have been cleared.";
    }
}

void ITunesFeature::onTrackCollectionLoaded(){
    TreeItem* root = m_future.result();
    if (root) {
        m_childModel.setRootItem(root);

        // Tell the rhythmbox track source that it should re-build its index.
        m_pTrackCollection->getTrackSource("itunes")->buildIndex();

        //m_pITunesTrackModel->select();
        emit(showTrackModel(m_pITunesTrackModel));
        qDebug() << "Itunes library loaded: success";
    } else {
        QMessageBox::warning(
            NULL,
            tr("Error Loading iTunes Library"),
            tr("There was an error loading your iTunes library. Some of "
               "your iTunes tracks or playlists may not have loaded."));
    }
    // calls a slot in the sidebarmodel such that 'isLoading' is removed from the feature title.
    m_title = tr("iTunes");
    emit(featureLoadingFinished(this));
    activate();
}

void ITunesFeature::onLazyChildExpandation(const QModelIndex &index){
	Q_UNUSED(index);
	//Nothing to do because the childmodel is not of lazy nature.
}

void ITunesFeature::slotAddToAutoDJ() {
    //qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();
	addToAutoDJ(false); // Top = True
}


void ITunesFeature::slotAddToAutoDJTop() {
    //qDebug() << "slotAddToAutoDJTop() row:" << m_lastRightClickedIndex.data();
	addToAutoDJ(true); // bTop = True
}

void ITunesFeature::addToAutoDJ(bool bTop) {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {

    	QString playlist = m_lastRightClickedIndex.data().toString();
    	ITunesPlaylistModel* pPlaylistModelToAdd = new ITunesPlaylistModel(this, m_pTrackCollection);
    	pPlaylistModelToAdd->setPlaylist(playlist);
    	pPlaylistModelToAdd->select();
    	PlaylistDAO &playlistDao = m_pTrackCollection->getPlaylistDAO();
        int autoDJId = playlistDao.getPlaylistIdFromName(AUTODJ_TABLE);

        int rows = pPlaylistModelToAdd->rowCount();
        for(int i = 0; i < rows; ++i){
            QModelIndex index = pPlaylistModelToAdd->index(i,0);
            if (index.isValid()) {
            	qDebug() << pPlaylistModelToAdd->getTrackLocation(index);
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

void ITunesFeature::slotImportAsMixxxPlaylist() {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {

    	QString playlist = m_lastRightClickedIndex.data().toString();
    	ITunesPlaylistModel* pPlaylistModelToAdd = new ITunesPlaylistModel(this, m_pTrackCollection);
    	pPlaylistModelToAdd->setPlaylist(playlist);
    	pPlaylistModelToAdd->select();
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
                	qDebug() << pPlaylistModelToAdd->getTrackLocation(index);
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
