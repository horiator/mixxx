#include <QtCore>
#include <QtGui>
#include <QtSql>
#include <QtDebug>

#include "trackcollection.h"

#include "defs.h"
#include "library/librarytablemodel.h"
#include "library/schemamanager.h"
#include "soundsourceproxy.h"
#include "trackinfoobject.h"
#include "xmlparse.h"

TrackCollection::TrackCollection(ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig),
          m_db(QSqlDatabase::addDatabase("QSQLITE")), // defaultConnection
          m_playlistDao(m_db),
          m_cueDao(m_db),
          m_trackDao(m_db, m_cueDao, pConfig),
          m_crateDao(m_db),
          m_supportedFileExtensionsRegex(
              SoundSourceProxy::supportedFileExtensionsRegex(),
              Qt::CaseInsensitive) {
    bCancelLibraryScan = false;
    qDebug() << "Available QtSQL drivers:" << QSqlDatabase::drivers();
    m_db.setHostName("localhost");
    m_db.setDatabaseName(MIXXX_DB_PATH);
    m_db.setUserName("mixxx");
    m_db.setPassword("mixxx");
    bool ok = m_db.open();
    qDebug() << __FILE__ << "DB status:" << ok;
    if (m_db.lastError().isValid()) {
        qDebug() << "Error loading database:" << m_db.lastError();
    }
    // Check for tables and create them if missing
    if (!checkForTables()) {
        // TODO(XXX) something a little more elegant
        exit(-1);
    }
}

TrackCollection::~TrackCollection() {
    qDebug() << "~TrackCollection()";
    // Save all tracks that haven't been saved yet.
	// and reset played flags
    m_trackDao.finish();

    Q_ASSERT(!m_db.rollback()); //Rollback any uncommitted transaction
    //The above is an ASSERT because there should never be an outstanding
    //transaction when this code is called. If there is, it means we probably
    //aren't committing a transaction somewhere that should be.
    m_db.close();
}

bool TrackCollection::checkForTables() {
    if (!m_db.open()) {
        QMessageBox::critical(0, tr("Cannot open database"),
                              tr("Unable to establish a database connection.\n"
                                 "Mixxx requires QT with SQLite support. Please read "
                                 "the Qt SQL driver documentation for information on how "
                                 "to build it.\n\n"
                                 "Click OK to exit."), QMessageBox::Ok);
        return false;
    }

    int requiredSchemaVersion = 12;
    if (!SchemaManager::upgradeToSchemaVersion(m_pConfig, m_db,
                                               requiredSchemaVersion)) {
        QMessageBox::warning(0, tr("Cannot upgrade database schema"),
                             tr("Unable to upgrade your database schema to version %1.\n"
                                "Your mixxx.db file may be corrupt.\n"
                                "Try renaming it and restarting Mixxx.\n\n"
                                "Click OK to exit.").arg(requiredSchemaVersion),
                             QMessageBox::Ok);
        return false;
    }

    m_trackDao.initialize();
    m_playlistDao.initialize();
    m_crateDao.initialize();
    m_cueDao.initialize();

    return true;
}

QSqlDatabase& TrackCollection::getDatabase() {
    return m_db;
}

/** Do a non-recursive import of all the songs in a directory. Does NOT decend into subdirectories.
    @param trackDao The track data access object which provides a connection to the database. We use this parameter in order to make this function callable from separate threads. You need to use a different DB connection for each thread.
    @return true if the scan completed without being cancelled. False if the scan was cancelled part-way through.
*/
bool TrackCollection::importDirectory(QString directory, TrackDAO &trackDao,
										const QStringList & nameFilters) {
    //qDebug() << "TrackCollection::importDirectory(" << directory<< ")";

    emit(startedLoading());
    QFileInfoList files;

    //get a list of the contents of the directory and go through it.
    QDirIterator it(directory, nameFilters, QDir::Files | QDir::NoDotAndDotDot);
    while (it.hasNext()) {

        //If a flag was raised telling us to cancel the library scan then stop.
        m_libraryScanMutex.lock();
        bool cancel = bCancelLibraryScan;
        m_libraryScanMutex.unlock();
        if (cancel) {
            return false;
        }

      	QString absoluteFilePath = it.next();

		//If the track is in the database, mark it as existing. This code gets exectuted
		//when other files in the same directory have changed (the directory hash has changed).
		trackDao.markTrackLocationAsVerified(absoluteFilePath);

		// If the file already exists in the database, continue and go on to
		// the next file.

		// If the file doesn't already exist in the database, then add
		// it. If it does exist in the database, then it is either in the
		// user's library OR the user has "removed" the track via
		// "Right-Click -> Remove". These tracks stay in the library, but
		// their mixxx_deleted column is 1.
		if (!trackDao.trackExistsInDatabase(absoluteFilePath))
		{
			//qDebug() << "Loading" << it.fileName();
        	emit(progressLoading(it.fileName()));

			TrackInfoObject* pTrack = new TrackInfoObject(absoluteFilePath);
			trackDao.addTracksTrack(pTrack);
			// TODO: signal track has changed with the mixxx m_trackDao;
			// m_trackDao.databaseTrackChanged(pTrack);
			delete pTrack;
		}
    }
    emit(finishedLoading());
    return true;
}

void TrackCollection::slotCancelLibraryScan() {
    m_libraryScanMutex.lock();
    bCancelLibraryScan = 1;
    m_libraryScanMutex.unlock();
}

void TrackCollection::resetLibaryCancellation() {
    m_libraryScanMutex.lock();
    bCancelLibraryScan = 0;
    m_libraryScanMutex.unlock();
}

CrateDAO& TrackCollection::getCrateDAO() {
    return m_crateDao;
}

TrackDAO& TrackCollection::getTrackDAO() {
    return m_trackDao;
}

PlaylistDAO& TrackCollection::getPlaylistDAO() {
    return m_playlistDao;
}

QSharedPointer<BaseTrackCache> TrackCollection::getTrackSource(
    const QString name) {
    return m_trackSources.value(name, QSharedPointer<BaseTrackCache>());
}

void TrackCollection::addTrackSource(
    const QString name, QSharedPointer<BaseTrackCache> trackSource) {
    Q_ASSERT(!m_trackSources.contains(name));
    m_trackSources[name] = trackSource;
}
