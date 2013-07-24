// autodjfeature.cpp
// FORK FORK FORK on 11/1/2009 by Albert Santoni (alberts@mixxx.org)
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "library/autodj/autodjfeature.h"
#include "library/playlisttablemodel.h"

#include "library/trackcollection.h"
#include "dlgautodj.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"
#include "mixxxkeyboard.h"
#include "soundsourceproxy.h"

const QString AutoDJFeature::m_sAutoDJViewName = QString("Auto DJ");

AutoDJFeature::AutoDJFeature(QObject* parent,
                             ConfigObject<ConfigValue>* pConfig,
                             TrackCollection* pTrackCollection)
        : LibraryFeature(parent),
          m_pConfig(pConfig),
          m_pTrackCollection(pTrackCollection),
          m_playlistDao(pTrackCollection->getPlaylistDAO()),
          m_pAutoDJView(NULL) {
    m_pAutoDJ = new AutoDJ(this, m_pConfig, m_pTrackCollection);
    connect(m_pAutoDJ, SIGNAL(loadTrack(TrackPointer)),
            this, SIGNAL(loadTrack(TrackPointer)));
    connect(m_pAutoDJ, SIGNAL(loadTrackToPlayer(TrackPointer, QString)),
            this, SIGNAL(loadTrackToPlayer(TrackPointer, QString)));
}

AutoDJFeature::~AutoDJFeature() {
}

QVariant AutoDJFeature::title() {
    return tr("Auto DJ");
}

QIcon AutoDJFeature::getIcon() {
    return QIcon(":/images/library/ic_library_autodj.png");
}

void AutoDJFeature::bindWidget(WLibrary* libraryWidget,
                               MixxxKeyboard* keyboard) {
    m_pAutoDJView = new DlgAutoDJ(libraryWidget,
                                  m_pConfig,
                                  m_pTrackCollection,
                                  keyboard, m_pAutoDJ);
    m_pAutoDJView->installEventFilter(keyboard);
    libraryWidget->registerView(m_sAutoDJViewName, m_pAutoDJView);
    //connect(m_pAutoDJView, SIGNAL(loadTrack(TrackPointer)),
            //this, SIGNAL(loadTrack(TrackPointer)));
    //connect(m_pAutoDJView, SIGNAL(loadTrackToPlayer(TrackPointer, QString)),
            //this, SIGNAL(loadTrackToPlayer(TrackPointer, QString)));
		m_pAutoDJ->setDlgAutoDJ(m_pAutoDJView);
}

TreeItemModel* AutoDJFeature::getChildModel() {
    return &m_childModel;
}

void AutoDJFeature::activate() {
    //qDebug() << "AutoDJFeature::activate()";
    emit(switchToView(m_sAutoDJViewName));
    emit(restoreSearch(QString())); //Null String disables search box
}

bool AutoDJFeature::dragMoveAccept(QUrl url) {
    QFileInfo file(url.toLocalFile());
    return SoundSourceProxy::isFilenameSupported(file.fileName());
}
