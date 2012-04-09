/***************************************************************************
                          trackinfoobject.cpp  -  description
                             -------------------
    begin                : 10 02 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QDomNode>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QMutexLocker>
#include <QString>
#include <QtDebug>

#include "trackinfoobject.h"

#include "soundsourceproxy.h"
#include "xmlparse.h"
#include "controlobject.h"

#include "mixxxutils.cpp"

TrackInfoObject::TrackInfoObject(const QString& file, bool parseHeader)
        : m_fileInfo(file),
          m_dateAdded(QDateTime::currentDateTime()),
          m_qMutex(QMutex::Recursive) {
    initialize(parseHeader);
}

TrackInfoObject::TrackInfoObject(const QFileInfo& fileInfo, bool parseHeader)
        : m_fileInfo(fileInfo),
          m_dateAdded(QDateTime::currentDateTime()),
          m_qMutex(QMutex::Recursive) {    
    initialize(parseHeader);
}

TrackInfoObject::TrackInfoObject(const QDomNode &nodeHeader)
        : m_qMutex(QMutex::Recursive) {
    QString filename = XmlParse::selectNodeQString(nodeHeader, "Filename");
    QString location = XmlParse::selectNodeQString(nodeHeader, "Filepath") + "/" +  filename;    
	m_fileInfo = QFileInfo(location);

    // We don't call initialize() here because it would end up calling parse()
    // on the file. Plus those initializations weren't done before, so it might
    // cause subtle bugs. This constructor is only used for legacy importing so
    // I'm not going to do it. rryan 6/2010

    m_sTitle = XmlParse::selectNodeQString(nodeHeader, "Title");
    m_sArtist = XmlParse::selectNodeQString(nodeHeader, "Artist");
    m_sType = XmlParse::selectNodeQString(nodeHeader, "Type");
    m_sComment = XmlParse::selectNodeQString(nodeHeader, "Comment");
    m_iDuration = XmlParse::selectNodeQString(nodeHeader, "Duration").toInt();
    m_iSampleRate = XmlParse::selectNodeQString(nodeHeader, "SampleRate").toInt();
    m_iChannels = XmlParse::selectNodeQString(nodeHeader, "Channels").toInt();
    m_iBitrate = XmlParse::selectNodeQString(nodeHeader, "Bitrate").toInt();
    //m_iLength = XmlParse::selectNodeQString(nodeHeader, "Length").toInt();
    m_iTimesPlayed = XmlParse::selectNodeQString(nodeHeader, "TimesPlayed").toInt();
    m_fReplayGain = XmlParse::selectNodeQString(nodeHeader, "replaygain").toFloat();
    m_fBpm = XmlParse::selectNodeQString(nodeHeader, "Bpm").toFloat();
    m_bBpmConfirm = XmlParse::selectNodeQString(nodeHeader, "BpmConfirm").toInt();
    m_fBeatFirst = XmlParse::selectNodeQString(nodeHeader, "BeatFirst").toFloat();
    m_bHeaderParsed = false;
    /*
    QString create_date = XmlParse::selectNodeQString(nodeHeader, "CreateDate");
    if (create_date == "")
        m_dCreateDate = m_fileInfo.created();
    else
        m_dCreateDate = QDateTime::fromString(create_date);
    */

    // Mixxx <1.8 recorded track IDs in mixxxtrack.xml, but we are going to
    // ignore those. Tracks will get a new ID from the database.
    //m_iId = XmlParse::selectNodeQString(nodeHeader, "Id").toInt();
    m_iId = -1;

    m_fCuePoint = XmlParse::selectNodeQString(nodeHeader, "CuePoint").toFloat();
    m_bPlayed = false;

    m_pVisualWave = 0;
    m_dVisualResampleRate = 0;

    //m_pWave = XmlParse::selectNodeHexCharArray(nodeHeader, QString("WaveSummaryHex"));

    m_bIsValid = true;

    m_bDirty = false;
    m_bLocationChanged = false;
}

void TrackInfoObject::initialize(bool parseHeader) {
    m_bDirty = false;
    m_bLocationChanged = false;

    //m_sArtist = "";
    //m_sTitle = "";
    //m_sType= "";
    //m_sComment = "";
    //m_sYear = "";
    //m_sURL = "";
    m_iDuration = 0;
    m_iBitrate = 0;
    m_iTimesPlayed = 0;
    m_bPlayed = false;
    m_fBpm = 0.;
    m_fReplayGain = 0.;
    m_bBpmConfirm = false;
    //m_bIsValid = false;
    m_bHeaderParsed = false;
    m_fBeatFirst = -1.;
    m_iId = -1;
    m_pVisualWave = 0;
    m_iSampleRate = 0;
    m_iChannels = 0;
    m_fCuePoint = 0.0f;
    m_dVisualResampleRate = 0;
    //m_dCreateDate =
    //m_dateAdded = QDateTime::currentDateTime();
    m_Rating = 0;
    //m_key = "";

    // parse() parses the metadata from file. This is not a quick operation!
    if (parseHeader) {
        parse();
    }
}

TrackInfoObject::~TrackInfoObject() {
    //qDebug() << "~TrackInfoObject()" << m_iId << getInfo();
}

void TrackInfoObject::doSave() {
    //qDebug() << "TIO::doSave()" << getInfo();
    emit(save(this));
}

/*
bool TrackInfoObject::isValid() const {
    QMutexLocker lock(&m_qMutex);
    return m_bIsValid;
}
*/

/*
    Writes information about the track to the xml file:
 */
/*
void TrackInfoObject::writeToXML( QDomDocument &doc, QDomElement &header )
{
    QMutexLocker lock(&m_qMutex);

    QString create_date;
    XmlParse::addElement( doc, header, "Filename", m_fileInfo.fileName() );
    //XmlParse::addElement( doc, header, "Filepath", m_sFilepath );
    XmlParse::addElement( doc, header, "Title", m_sTitle );
    XmlParse::addElement( doc, header, "Artist", m_sArtist );
    XmlParse::addElement( doc, header, "Type", m_sType );
    XmlParse::addElement( doc, header, "Comment", m_sComment);
    XmlParse::addElement( doc, header, "Duration", QString("%1").arg(m_iDuration));
    XmlParse::addElement( doc, header, "SampleRate", QString("%1").arg(m_iSampleRate));
    XmlParse::addElement( doc, header, "Channels", QString("%1").arg(m_iChannels));
    XmlParse::addElement( doc, header, "Bitrate", QString("%1").arg(m_iBitrate));
    XmlParse::addElement( doc, header, "Length", QString("%1").arg(m_fileInfo.size()) );
    XmlParse::addElement( doc, header, "TimesPlayed", QString("%1").arg(m_iTimesPlayed) );
    XmlParse::addElement( doc, header, "replaygain", QString("%1").arg(m_fReplayGain) );
    XmlParse::addElement( doc, header, "Bpm", QString("%1").arg(m_fBpm) );
    XmlParse::addElement( doc, header, "BpmConfirm", QString("%1").arg(m_bBpmConfirm) );
    XmlParse::addElement( doc, header, "BeatFirst", QString("%1").arg(m_fBeatFirst) );
    XmlParse::addElement( doc, header, "Id", QString("%1").arg(m_iId) );
    XmlParse::addElement( doc, header, "CuePoint", QString::number(m_fCuePoint) );
    XmlParse::addElement( doc, header, "CreateDate", m_dCreateDate.toString() );
    //if (m_pWave) {
        //XmlParse::addHexElement(doc, header, "WaveSummaryHex", m_pWave);
    //}

}
*/


int TrackInfoObject::parse()
{
    QMutexLocker lock(&m_qMutex);

    // Parse the using information stored in the sound file
    int result = SoundSourceProxy::ParseHeader(this);

    if (m_bHeaderParsed) {
        // Add basic information derived from the filename:
        parseFilename();
    }

    return result; // OK if Mixxx can handle this file
}


void TrackInfoObject::parseFilename()
{
	QString filename = m_fileInfo.fileName(); 

    if (filename.indexOf('-') != -1) {
        m_sArtist = filename.section('-',0,0).trimmed(); // Get the first part
        m_sTitle = filename.section('-',1,1); // Get the second part
        m_sTitle = m_sTitle.section('.',0,-2).trimmed(); // Remove the ending
    } else {
        m_sTitle = filename.section('.',0,-2).trimmed(); // Remove the ending
    }

    if (m_sTitle.isEmpty()) {
        m_sTitle = filename.section('.',0,-2).trimmed();
    }

    // Add no comment
    m_sComment.clear();

    // Find the type
    m_sType = filename.section(".",-1).toLower().trimmed();
    setDirty(true);
}

QString TrackInfoObject::getDurationStr() const
{
    QMutexLocker lock(&m_qMutex);
    int iDuration = m_iDuration;
    lock.unlock();

    return MixxxUtils::secondsToMinutes(iDuration, true);
}

void TrackInfoObject::setLocation(QString location)
{
    QMutexLocker lock(&m_qMutex);
    QFileInfo newFileInfo = QFileInfo(location);
    if (newFileInfo != m_fileInfo) {
		m_fileInfo = newFileInfo; 
        m_bLocationChanged = true;
        setDirty(true);
    }
}

QString TrackInfoObject::getLocation() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fileInfo.absoluteFilePath();
}

QString TrackInfoObject::getDirectory() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fileInfo.absolutePath();
}

QString TrackInfoObject::getFilename()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_fileInfo.fileName();
}

/*
QDateTime TrackInfoObject::getCreateDate() const
{
    QMutexLocker lock(&m_qMutex);
    QDateTime create_date = QDateTime(m_dCreateDate);
    return create_date;
}
*/

bool TrackInfoObject::exists()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_fileInfo.exists();
}

float TrackInfoObject::getReplayGain() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fReplayGain;
}

void TrackInfoObject::setReplayGain(float f)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_fReplayGain != f;
    m_fReplayGain = f;
    //qDebug() << "Reported ReplayGain value: " << m_fReplayGain;
    if (dirty)
        setDirty(true);
    lock.unlock();
    emit(ReplayGainUpdated(f));
}

float TrackInfoObject::getBpm() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fBpm;
}



void TrackInfoObject::setBpm(float f)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_fBpm != f;
    m_fBpm = f;
    if (dirty)
        setDirty(true);

    lock.unlock();
    //Tell the GUI to update the bpm label...
    //qDebug() << "TrackInfoObject signaling BPM update to" << f;
    emit(bpmUpdated(f));
}

QString TrackInfoObject::getBpmStr() const
{
    return QString("%1").arg(getBpm(), 3,'f',1);
}

bool TrackInfoObject::getBpmConfirm()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_bBpmConfirm;
}

void TrackInfoObject::setBpmConfirm(bool confirm)
{
    QMutexLocker lock(&m_qMutex);
    m_bBpmConfirm = confirm;
}

void TrackInfoObject::setBeats(BeatsPointer pBeats) {
    QMutexLocker lock(&m_qMutex);

    // This whole method is not so great. The fact that Beats is an ABC is
    // limiting with respect to QObject and signals/slots.

    QObject* pObject = NULL;
    if (m_pBeats) {
        pObject = dynamic_cast<QObject*>(m_pBeats.data());
        if (pObject)
            pObject->disconnect(this, SIGNAL(updated()));
    }
    m_pBeats = pBeats;
    pObject = dynamic_cast<QObject*>(m_pBeats.data());
    Q_ASSERT(pObject);
    if (pObject) {
        connect(pObject, SIGNAL(updated()),
                this, SLOT(slotBeatsUpdated()));
    }
    setDirty(true);
    lock.unlock();
    emit(beatsUpdated());
}

BeatsPointer TrackInfoObject::getBeats() const {
    QMutexLocker lock(&m_qMutex);
    return m_pBeats;
}

void TrackInfoObject::slotBeatsUpdated() {
    QMutexLocker lock(&m_qMutex);
    setDirty(true);
    lock.unlock();
    emit(beatsUpdated());
}

bool TrackInfoObject::getHeaderParsed()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_bHeaderParsed;
}

void TrackInfoObject::setHeaderParsed(bool parsed)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_bHeaderParsed != parsed;
    m_bHeaderParsed = parsed;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getInfo()  const
{
    QMutexLocker lock(&m_qMutex);
    QString artist = m_sArtist.trimmed() == "" ? "" : m_sArtist + ", ";
    QString sInfo = artist + m_sTitle;
    return sInfo;
}

QDateTime TrackInfoObject::getDateAdded() const {
    QMutexLocker lock(&m_qMutex);
    return m_dateAdded;
}

void TrackInfoObject::setDateAdded(const QDateTime& dateAdded) {
    QMutexLocker lock(&m_qMutex);
    m_dateAdded = dateAdded;
}

int TrackInfoObject::getDuration()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_iDuration;
}

void TrackInfoObject::setDuration(int i)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_iDuration != i;
    m_iDuration = i;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getTitle()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sTitle;
}

void TrackInfoObject::setTitle(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    QString title = s.trimmed();
    if (m_sTitle != title) {
        m_sTitle = s;
        setDirty(true);
    }
}

QString TrackInfoObject::getArtist()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sArtist;
}

void TrackInfoObject::setArtist(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    QString artist = s.trimmed();
    if (m_sArtist != artist) {
        m_sArtist != artist;
        setDirty(true);
    }
}

QString TrackInfoObject::getAlbum()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sAlbum;
}

void TrackInfoObject::setAlbum(QString s)
{
    QMutexLocker lock(&m_qMutex);
    s = s.trimmed();
    bool dirty = m_sAlbum != s;
    m_sAlbum = s;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getYear()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sYear;
}

void TrackInfoObject::setYear(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    QString year = s.trimmed();
    if (m_sYear != year) {
        m_sYear = year;
        setDirty(true);
    }
}

QString TrackInfoObject::getGenre()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sGenre;
}

void TrackInfoObject::setGenre(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    QString genre = s.trimmed();
    if (m_sGenre != genre) {
        m_sGenre = genre;
        setDirty(true);
    }
}

QString TrackInfoObject::getComposer()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sComposer;
}

void TrackInfoObject::setComposer(QString s)
{
    QMutexLocker lock(&m_qMutex);
    s = s.trimmed();
    bool dirty = m_sComposer != s;
    m_sComposer = s;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getTrackNumber()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_sTrackNumber;
}

void TrackInfoObject::setTrackNumber(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    QString tn = s.trimmed();
    if (m_sTrackNumber != tn) {
        m_sTrackNumber = tn;
        setDirty(true);
    }
}

int TrackInfoObject::getTimesPlayed()  const
{
    QMutexLocker lock(&m_qMutex);
    return m_iTimesPlayed;
}

void TrackInfoObject::setTimesPlayed(int t)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = t != m_iTimesPlayed;
    m_iTimesPlayed = t;
    if (dirty)
        setDirty(true);
}

void TrackInfoObject::incTimesPlayed()
{
    setPlayedAndUpdatePlaycount(true);
}

bool TrackInfoObject::getPlayed() const
{
    QMutexLocker lock(&m_qMutex);
    bool bPlayed = m_bPlayed;
    return bPlayed;
}

void TrackInfoObject::setPlayedAndUpdatePlaycount(bool bPlayed)
{
    QMutexLocker lock(&m_qMutex);
    if (bPlayed) {
        ++m_iTimesPlayed;
        setDirty(true);
    }
    else if (m_bPlayed && !bPlayed) {
        --m_iTimesPlayed;
        setDirty(true);
    }
    m_bPlayed = bPlayed;
}

void TrackInfoObject::setPlayed(bool bPlayed)
{
    QMutexLocker lock(&m_qMutex);
    if (bPlayed != m_bPlayed) {
        m_bPlayed = bPlayed;
        setDirty(true);
    }
}

QString TrackInfoObject::getComment() const
{
    QMutexLocker lock(&m_qMutex);
    return m_sComment;
}

void TrackInfoObject::setComment(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    if (s != m_sComment) {
        m_sComment = s;
        setDirty(true);
    }
}

QString TrackInfoObject::getType() const
{
    QMutexLocker lock(&m_qMutex);
    return m_sType;
}

void TrackInfoObject::setType(const QString& s)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = s != m_sType;
    m_sType = s;
    if (dirty)
        setDirty(true);
}

void TrackInfoObject::setSampleRate(int iSampleRate)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_iSampleRate != iSampleRate;
    m_iSampleRate = iSampleRate;
    if (dirty)
        setDirty(true);
}

int TrackInfoObject::getSampleRate() const
{
    QMutexLocker lock(&m_qMutex);
    return m_iSampleRate;
}

void TrackInfoObject::setChannels(int iChannels)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_iChannels != iChannels;
    m_iChannels = iChannels;
    if (dirty)
        setDirty(true);
}

int TrackInfoObject::getChannels() const
{
    QMutexLocker lock(&m_qMutex);
    return m_iChannels;
}

int TrackInfoObject::getLength() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fileInfo.size();
}

int TrackInfoObject::getBitrate() const
{
    QMutexLocker lock(&m_qMutex);
    return m_iBitrate;
}

QString TrackInfoObject::getBitrateStr() const
{
    return QString("%1").arg(getBitrate());
}

void TrackInfoObject::setBitrate(int i)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_iBitrate != i;
    m_iBitrate = i;
    if (dirty)
        setDirty(true);
}

void TrackInfoObject::setBeatFirst(float fBeatFirstPos)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_fBeatFirst != fBeatFirstPos;
    m_fBeatFirst = fBeatFirstPos;
    if (dirty)
        setDirty(true);
}

float TrackInfoObject::getBeatFirst() const
{
    QMutexLocker lock(&m_qMutex);
    return m_fBeatFirst;
}

int TrackInfoObject::getId() const {
    QMutexLocker lock(&m_qMutex);
    return m_iId;
}

void TrackInfoObject::setId(int iId) {
    QMutexLocker lock(&m_qMutex);
    // changing the Id does not make the track drity because the Id is always
    // generated by the Database itself
    //bool dirty = m_iId != iId;
    m_iId = iId;
    //if (dirty)
    //    setDirty(true);
}

QVector<float> * TrackInfoObject::getVisualWaveform() {
    QMutexLocker lock(&m_qMutex);
    return m_pVisualWave;
}

void TrackInfoObject::setVisualResampleRate(double dVisualResampleRate) {
    // Temporary, shared value that should not be saved. The only reason it
    // exists on the TIO is a temporary hack, so it does not dirty the TIO.
    QMutexLocker lock(&m_qMutex);
    m_dVisualResampleRate = dVisualResampleRate;
}

double TrackInfoObject::getVisualResampleRate() {
    QMutexLocker lock(&m_qMutex);
    return m_dVisualResampleRate;
}

const QByteArray *TrackInfoObject::getWaveSummary()
{
    QMutexLocker lock(&m_qMutex);
    return &m_waveSummary;
}

void TrackInfoObject::setVisualWaveform(QVector<float> *pWave) {
    // The visual waveform is not serialized currently so it does not dirty a
    // TIO.
    QMutexLocker lock(&m_qMutex);
    m_pVisualWave = pWave;
}

void TrackInfoObject::setWaveSummary(const QByteArray* pWave, bool updateUI)
{
    QMutexLocker lock(&m_qMutex);
    m_waveSummary = *pWave; //_Copy_ the bytes
    setDirty(true);
    lock.unlock();
    emit(wavesummaryUpdated(this));
}

void TrackInfoObject::setURL(QString url)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_sURL != url;
    m_sURL = url;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getURL()
{
    QMutexLocker lock(&m_qMutex);
    return m_sURL;
}

void TrackInfoObject::setCuePoint(float cue)
{
    QMutexLocker lock(&m_qMutex);
    bool dirty = m_fCuePoint != cue;
    m_fCuePoint = cue;
    if (dirty)
        setDirty(true);
}

float TrackInfoObject::getCuePoint()
{
    QMutexLocker lock(&m_qMutex);
    return m_fCuePoint;
}

void TrackInfoObject::slotCueUpdated() {
    setDirty(true);
    emit(cuesUpdated());
}

Cue* TrackInfoObject::addCue() {
    //qDebug() << "TrackInfoObject::addCue()";
    QMutexLocker lock(&m_qMutex);
    Cue* cue = new Cue(m_iId);
    connect(cue, SIGNAL(updated()),
            this, SLOT(slotCueUpdated()));
    m_cuePoints.push_back(cue);
    setDirty(true);
    lock.unlock();
    emit(cuesUpdated());
    return cue;
}

void TrackInfoObject::removeCue(Cue* cue) {
    QMutexLocker lock(&m_qMutex);
    disconnect(cue, 0, this, 0);
    m_cuePoints.removeOne(cue);
    setDirty(true);
    lock.unlock();
    emit(cuesUpdated());
}

const QList<Cue*>& TrackInfoObject::getCuePoints() {
    QMutexLocker lock(&m_qMutex);
    return m_cuePoints;
}

void TrackInfoObject::setCuePoints(QList<Cue*> cuePoints) {
    //qDebug() << "setCuePoints" << cuePoints.length();
    QMutexLocker lock(&m_qMutex);
    QListIterator<Cue*> it(m_cuePoints);
    while (it.hasNext()) {
        Cue* cue = it.next();
        disconnect(cue, 0, this, 0);
    }
    m_cuePoints = cuePoints;
    it = QListIterator<Cue*>(m_cuePoints);
    while (it.hasNext()) {
        Cue* cue = it.next();
        connect(cue, SIGNAL(updated()),
            this, SLOT(slotCueUpdated()));
    }
    setDirty(true);
    lock.unlock();
    emit(cuesUpdated());
}

const Segmentation<QString>* TrackInfoObject::getChordData() {
    QMutexLocker lock(&m_qMutex);
    return &m_chordData;
}

void TrackInfoObject::setChordData(Segmentation<QString> cd) {
    QMutexLocker lock(&m_qMutex);
    m_chordData = cd;
    setDirty(true);
}

void TrackInfoObject::setDirty(bool bDirty) {

    QMutexLocker lock(&m_qMutex);
    bool change = m_bDirty != bDirty;
    m_bDirty = bDirty;
    lock.unlock();
    // qDebug() << "Track" << m_iId << getInfo() << (change? "changed" : "unchanged")
    //          << "set" << (bDirty ? "dirty" : "clean");
    if (change) {
        if (m_bDirty)
            emit(dirty(this));
        else
            emit(clean(this));
    }
    // Emit a changed signal regardless if this attempted to set us dirty.
    if (bDirty) {
    	emit(changed(this));
    }

    //qDebug() << QString("TrackInfoObject %1 %2 set to %3").arg(QString::number(m_iId), m_fileInfo.absoluteFilePath(), m_bDirty ? "dirty" : "clean");
}

bool TrackInfoObject::isDirty() {
    QMutexLocker lock(&m_qMutex);
    return m_bDirty;
}

bool TrackInfoObject::locationChanged() {
    QMutexLocker lock(&m_qMutex);
    return m_bLocationChanged;
}

int TrackInfoObject::getRating() const{
    QMutexLocker lock(&m_qMutex);
    return m_Rating;
}

void TrackInfoObject::setRating (int rating){
    QMutexLocker lock(&m_qMutex);
    bool dirty = rating != m_Rating;
    m_Rating = rating;
    if (dirty)
        setDirty(true);
}

QString TrackInfoObject::getKey() const{
    QMutexLocker lock(&m_qMutex);
    return m_key;
}

void TrackInfoObject::setKey(QString key){
    QMutexLocker lock(&m_qMutex);
    bool dirty = key != m_key;
    m_key = key;
    if (dirty)
        setDirty(true);
}

#ifdef __TAGREADER__
void TrackInfoObject::InitFromProtobuf(const pb::tagreader::SongMetadata& pb) {
  /*
    d->init_from_file_ = true;
  d->valid_ = pb.valid();
  d->title_ = QStringFromStdString(pb.title());
  d->album_ = QStringFromStdString(pb.album());
  d->artist_ = QStringFromStdString(pb.artist());
  d->albumartist_ = QStringFromStdString(pb.albumartist());
  d->composer_ = QStringFromStdString(pb.composer());
  d->track_ = pb.track();
  d->disc_ = pb.disc();
  d->bpm_ = pb.bpm();
  d->year_ = pb.year();
  d->genre_ = QStringFromStdString(pb.genre());
  d->comment_ = QStringFromStdString(pb.comment());
  d->compilation_ = pb.compilation();
  d->playcount_ = pb.playcount();
  d->skipcount_ = pb.skipcount();
  d->lastplayed_ = pb.lastplayed();
  d->score_ = pb.score();
  set_length_nanosec(pb.length_nanosec());
  d->bitrate_ = pb.bitrate();
  d->samplerate_ = pb.samplerate();
  d->url_ = QUrl::fromEncoded(QByteArray(pb.url().data(), pb.url().size()));
  d->basefilename_ = QStringFromStdString(pb.basefilename());
  d->mtime_ = pb.mtime();
  d->ctime_ = pb.ctime();
  d->filesize_ = pb.filesize();
  d->suspicious_tags_ = pb.suspicious_tags();
  d->art_automatic_ = QStringFromStdString(pb.art_automatic());
  d->filetype_ = static_cast<FileType>(pb.type());

  if (pb.has_rating()) {
    d->rating_ = pb.rating();
  }
  */
}

void TrackInfoObject::ToProtobuf(pb::tagreader::SongMetadata* pb) const {

    QUrl temp_url(m_fileInfo.absoluteFilePath());
    const QByteArray url(temp_url.toEncoded());

    //pb->set_valid(d->valid_); // extension is valid
    pb->set_title(DataCommaSizeFromQString(m_sTitle));
    pb->set_album(DataCommaSizeFromQString(m_sAlbum));
    pb->set_artist(DataCommaSizeFromQString(m_sArtist));
    //pb->set_albumartist(DataCommaSizeFromQString(d->albumartist_));
    pb->set_composer(DataCommaSizeFromQString(m_sComposer));
    pb->set_track(m_sTrackNumber.toInt());
    //pb->set_disc(d->disc_);
    pb->set_bpm(m_fBpm);
    pb->set_year(m_sYear.toInt());
    pb->set_genre(DataCommaSizeFromQString(m_sGenre));
    pb->set_comment(DataCommaSizeFromQString(m_sComment));
    //pb->set_compilation(d->compilation_);
    pb->set_rating((float) m_Rating);
    //pb->set_playcount(d->playcount_);
    //pb->set_skipcount(d->skipcount_);
    //pb->set_lastplayed(d->lastplayed_);
    //pb->set_score(d->score_);
    m_iDuration
    // pb->set_length_nanosec(length_nanosec());
    pb->set_bitrate(m_iBitrate);
    pb->set_samplerate(m_iSampleRate);
    pb->set_url(url.constData(), url.size());
    //pb->set_basefilename(DataCommaSizeFromQString(d->basefilename_));
    //pb->set_mtime(d->mtime_);
    //pb->set_ctime(d->ctime_);
    pb->set_filesize(m_fileInfo.size());
    //pb->set_suspicious_tags(d->suspicious_tags_);
    //pb->set_art_automatic(DataCommaSizeFromQString(d->art_automatic_));
    //pb->set_type(static_cast< ::pb::tagreader::SongMetadata_Type>(d->filetype_));
}
#endif // __TAGREADER__
