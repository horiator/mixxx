#include <QtCore>
#include <QtGui>
#include <QtSql>

#include "library/itunes/itunestrackmodel.h"
#include "library/trackcollection.h"
#include "track/beatfactory.h"
#include "track/beats.h"

ITunesTrackModel::ITunesTrackModel(QObject* parent,
                                   TrackCollection* pTrackCollection)
        : BaseSqlTableModel(parent, pTrackCollection,
                            pTrackCollection->getDatabase(),
                            "mixxx.db.model.itunes"),
          m_pTrackCollection(pTrackCollection),
          m_database(m_pTrackCollection->getDatabase()) {
    connect(this, SIGNAL(doSearch(const QString&)),
            this, SLOT(slotSearch(const QString&)));
    QStringList columns;
    columns << "id";
    setTable("itunes_library", columns[0], columns,
             m_pTrackCollection->getTrackSource("itunes"));
    setDefaultSort(fieldIndex("artist"), Qt::AscendingOrder);
    initHeaderData();
}

ITunesTrackModel::~ITunesTrackModel() {
}

TrackPointer ITunesTrackModel::getTrack(const QModelIndex& index) const {
    QString artist = index.sibling(index.row(), fieldIndex("artist")).data().toString();
    QString title = index.sibling(index.row(), fieldIndex("title")).data().toString();
    QString album = index.sibling(index.row(), fieldIndex("album")).data().toString();
    QString year = index.sibling(index.row(), fieldIndex("year")).data().toString();
    QString genre = index.sibling(index.row(), fieldIndex("genre")).data().toString();
    float bpm = index.sibling(index.row(), fieldIndex("bpm")).data().toString().toFloat();

    QString location = index.sibling(index.row(), fieldIndex("location")).data().toString();

    if (location.isEmpty()) {
    	// Track is lost
    	return TrackPointer();
    }

    TrackDAO& track_dao = m_pTrackCollection->getTrackDAO();
    int track_id = track_dao.getTrackId(location);
    if (track_id < 0) {
    	// Add Track to library
    	track_id = track_dao.addTrack(location, true);
    }

    TrackPointer pTrack;

    if (track_id < 0) {
    	// Add Track to library failed
    	// Create own TrackInfoObject
    	pTrack = TrackPointer(new TrackInfoObject(location), &QObject::deleteLater);
    }
    else {
    	pTrack = track_dao.getTrack(track_id);
    }

    // Overwrite metadata from iTunes library
    // Note: This will be written to the mixxx library as well
    pTrack->setArtist(artist);
    pTrack->setTitle(title);
    pTrack->setAlbum(album);
    pTrack->setYear(year);
    pTrack->setGenre(genre);
    pTrack->setBpm(bpm);

    // If the track has a BPM, then give it a static beatgrid.
    if (bpm > 0) {
        BeatsPointer pBeats = BeatFactory::makeBeatGrid(pTrack, bpm, 0);
        pTrack->setBeats(pBeats);
    }

    return pTrack;
}

void ITunesTrackModel::search(const QString& searchText) {
    // qDebug() << "ITunesTrackModel::search()" << searchText
    //          << QThread::currentThread();
    emit(doSearch(searchText));
}

void ITunesTrackModel::slotSearch(const QString& searchText) {
    BaseSqlTableModel::search(searchText);
}

bool ITunesTrackModel::isColumnInternal(int column) {
    if (column == fieldIndex(LIBRARYTABLE_ID)) {
        return true;
    }
    return false;
}

Qt::ItemFlags ITunesTrackModel::flags(const QModelIndex &index) const {
    return readOnlyFlags(index);
}

bool ITunesTrackModel::isColumnHiddenByDefault(int column) {
	Q_UNUSED(column);
	return false;
}

TrackModel::CapabilitiesFlags ITunesTrackModel::getCapabilities() const {
    return TRACKMODELCAPS_NONE
            //| TRACKMODELCAPS_REORDER
            //| TRACKMODELCAPS_RECEIVEDROPS
            | TRACKMODELCAPS_ADDTOPLAYLIST
            | TRACKMODELCAPS_ADDTOCRATE
            | TRACKMODELCAPS_ADDTOAUTODJ
            //| TRACKMODELCAPS_LOCKED
            //| TRACKMODELCAPS_RELOADMETADATA
            | TRACKMODELCAPS_LOADTODECK
            | TRACKMODELCAPS_LOADTOSAMPLER
            //| TRACKMODELCAPS_REMOVE
            //| TRACKMODELCAPS_RELOCATE
            ;
}
