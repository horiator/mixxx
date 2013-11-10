// clementineview.cpp
// Created 2/25/2011 by Daniel Schürmann (daschuer@gmx.de)
//
// This shows the Clementine-Player library in Mixxx



#include "clementineview.h"
#include "clementinefeature.h"
#include "trackinfoobject.h"
#include "track/beatfactory.h"
#include "controlobjectthreadmain.h"
#include "controlobject.h"

// from clementine
#include "library/libraryviewcontainer.h"
#include "library/libraryview.h"
#include "library/library.h"
#include "library/librarymodel.h"
#include "library/libraryfilterwidget.h"
#include "core/taskmanager.h"
#include "playlist/songmimedata.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistsequence.h"
#include "playlist/playlistview.h"
#include "core/application.h"


#include <QSortFilterProxyModel>
#include <QModelIndexList>
#include <QSignalMapper>
#include <QAction>
#include <QMenu>
#include <QHBoxLayout>
#include <QSplitter>

const char* ClementineView::kSettingsGroup = "ClementineView";

ClementineView::ClementineView(QWidget* parent, ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection)
    : QWidget(parent),
      m_pClementineApp(NULL),
      m_libraryViewContainer(new LibraryViewContainer(this)),
      m_playlistContainer(new PlaylistContainer(this)),
      m_librarySortModel(new QSortFilterProxyModel(this)),
      m_pConfig(pConfig),
      m_pData(NULL),
      m_pTrackCollection(pTrackCollection),
      m_playlistAddToAnother(NULL) {
    /*
    m_horizontalLayout = new QHBoxLayout(this);
    m_horizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_horizontalLayout->addWidget(m_libraryViewContainer);
    */

    QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
    QSplitter* splitter = new QSplitter(this);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->addWidget(splitter);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(m_libraryViewContainer);
    splitter->addWidget(m_playlistContainer);
}

ClementineView::~ClementineView() {
}

void ClementineView::setApplication(Application* app) {
    m_pClementineApp = app;

    qDebug() << "Register Meta Types";
    //qRegisterMetaType<CoverSearchResult>("CoverSearchResult");
    //qRegisterMetaType<QList<CoverSearchResult> >("QList<CoverSearchResult>");
    //qRegisterMetaType<CoverSearchResults>("CoverSearchResults");
    qRegisterMetaType<Directory>("Directory");
    qRegisterMetaType<DirectoryList>("DirectoryList");
    qRegisterMetaType<Subdirectory>("Subdirectory");
    qRegisterMetaType<SubdirectoryList>("SubdirectoryList");
    qRegisterMetaType<Song>("Song");
    qRegisterMetaType<QList<Song> >("QList<Song>");
    qRegisterMetaType<SongList>("SongList");
    qRegisterMetaType<PlaylistItemPtr>("PlaylistItemPtr");
    qRegisterMetaType<QList<PlaylistItemPtr> >("QList<PlaylistItemPtr>");
    qRegisterMetaType<PlaylistItemList>("PlaylistItemList");
    //qRegisterMetaType<Engine::State>("Engine::State");
    //qRegisterMetaType<Engine::SimpleMetaBundle>("Engine::SimpleMetaBundle");
    //qRegisterMetaType<Equalizer::Params>("Equalizer::Params");
    //qRegisterMetaTypeStreamOperators<Equalizer::Params>("Equalizer::Params");
    qRegisterMetaType<const char*>("const char*");
    qRegisterMetaType<QNetworkReply*>("QNetworkReply*");
    qRegisterMetaType<QNetworkReply**>("QNetworkReply**");
    qRegisterMetaType<smart_playlists::GeneratorPtr>("smart_playlists::GeneratorPtr");
    qRegisterMetaType<ColumnAlignmentMap>("ColumnAlignmentMap");
    qRegisterMetaTypeStreamOperators<QMap<int, int> >("ColumnAlignmentMap");
    //qRegisterMetaType<QNetworkCookie>("QNetworkCookie");
    //qRegisterMetaType<QList<QNetworkCookie> >("QList<QNetworkCookie>");
    //qRegisterMetaType<SearchProvider::Result>("SearchProvider::Result");
    //qRegisterMetaType<SearchProvider::ResultList>("SearchProvider::ResultList");
    //qRegisterMetaType<DigitallyImportedClient::Channel>("DigitallyImportedClient::Channel");
    //qRegisterMetaType<SomaFMService::Stream>("SomaFMService::Stream");
    //qRegisterMetaTypeStreamOperators<DigitallyImportedClient::Channel>("DigitallyImportedClient::Channel");
    //qRegisterMetaTypeStreamOperators<SomaFMService::Stream>("SomaFMService::Stream");


    qDebug() << "Creating models";
    m_librarySortModel->setSourceModel((QAbstractItemModel*)(m_pClementineApp->library()->model()));
    m_librarySortModel->setSortRole(LibraryModel::Role_SortText);
    m_librarySortModel->setDynamicSortFilter(true);
    m_librarySortModel->sort(0);

    m_libraryViewContainer->view()->setModel((QAbstractItemModel *)m_librarySortModel);
    m_libraryViewContainer->view()->SetApplication(m_pClementineApp);

    // Is emmited also for doubleClick
    connect(m_libraryViewContainer->view(), SIGNAL(AddToPlaylistSignal(QMimeData*)),
            SLOT(slotAddToClementinePlaylist(QMimeData*)));

    connect(m_libraryViewContainer->view(), SIGNAL(RightClicked(QMimeData*)),
            SLOT(slotLibraryViewRightClicked(QMimeData*)));

    // Auto DJ Actions
    m_pAddToAutoDJAction = m_libraryViewContainer->view()->AddSongAction(QIcon(),
            tr("Add to Auto DJ bottom"), this, SLOT(slotSendToAutoDJ()));
    m_pAddToAutoDJTopAction = m_libraryViewContainer->view()->AddSongAction(QIcon(),
            tr("Add to Auto DJ top 2"), this, SLOT(slotSendToAutoDJTop()));

    m_libraryViewContainer->view()->AddSongActionSeparator();

    connect(&m_groupMapper, SIGNAL(mapped(QString)),
            SLOT(loadSelectionToGroup(QString)));

    ControlObjectThreadMain* pNumDecks = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Master]", "num_decks")));
    int iNumDecks = pNumDecks->get();
    delete pNumDecks;

    // Load to Deck Actions
    for (int i = 1; i <= iNumDecks; ++i) {
        QString deckGroup = QString("[Channel%1]").arg(i);
        QAction* pAction = m_libraryViewContainer->view()->AddSongAction(QIcon(),
                tr("Load to Deck %1").arg(i), &m_groupMapper, SLOT(map()));
        m_groupMapper.setMapping(pAction, deckGroup);
    }


    ControlObjectThreadMain* pNumSamplers = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Master]", "num_samplers")));
    int iNumSamplers = pNumSamplers->get();
    delete pNumSamplers;

    if (iNumSamplers > 0) {
        m_pMenuSampler = m_libraryViewContainer->view()->AddSongActionMenu();
        m_pMenuSampler->setTitle(tr("Load to Sampler"));
        for (int i = 1; i <= iNumSamplers; ++i) {
            QString samplerGroup = QString("[Sampler%1]").arg(i);
            QAction* pAction = new QAction(tr("Sampler %1").arg(i), m_pMenuSampler);
            m_pMenuSampler->addAction(pAction);
            m_groupMapper.setMapping(pAction, samplerGroup);
            connect(pAction, SIGNAL(triggered()), &m_groupMapper, SLOT(map()));
        }
    }

    m_libraryViewContainer->view()->AddSongActionSeparator();

    connect(&m_playlistMapper, SIGNAL(mapped(int)),
            this, SLOT(slotAddToMixxxPlaylist(int)));

    m_pMenuPlaylist = m_libraryViewContainer->view()->AddSongActionMenu();
    m_pMenuPlaylist->setTitle(tr("Add to Mixxx Playlist"));

    connect(&m_crateMapper, SIGNAL(mapped(int)),
            this, SLOT(addSelectionToCrate(int)));

    m_pMenuCrate = m_libraryViewContainer->view()->AddSongActionMenu();
    m_pMenuCrate->setTitle(tr("Add to Crate"));

    m_libraryViewContainer->view()->AddSongActionSeparator();

    m_libraryViewContainer->filter()->SetSettingsGroup(kSettingsGroup);
    m_libraryViewContainer->filter()->SetLibraryModel(m_pClementineApp->library()->model());

        /*
    QAction* separator = new QAction(this);
    separator->setSeparator(true);

    library_view_->filter()->AddMenuAction(library_show_all_);
    library_view_->filter()->AddMenuAction(library_show_duplicates_);
    library_view_->filter()->AddMenuAction(library_show_untagged_);
    library_view_->filter()->AddMenuAction(separator);
    library_view_->filter()->AddMenuAction(library_config_action);
    */

    /////////////////////////////////////////////
    // PlaylistContainer

    // connect(m_playlistContainer, SIGNAL(ViewSelectionModelChanged()), SLOT(PlaylistViewSelectionModelChanged()));

    m_playlistsManager =  m_pClementineApp->playlist_manager();
    m_playlistSequence = new PlaylistSequence(this);
    m_playlistSequence->hide();

    m_playlistContainer->SetManager(m_pClementineApp->playlist_manager());
    m_playlistContainer->view()->SetApplication(m_pClementineApp);

    // Load playlists
    m_playlistsManager->Init(m_pClementineApp->library_backend(), m_pClementineApp->playlist_backend(),
            m_playlistSequence, m_playlistContainer);

    // Create actions for buttons on Top of playlist
    QAction* m_actionNewPlaylist;
    QAction* m_actionSavePlaylist;
    QAction* m_actionLoadPlaylist;

    m_actionNewPlaylist = new QAction(this);
    m_actionSavePlaylist = new QAction(this);
    m_actionLoadPlaylist = new QAction(this);

    m_actionNewPlaylist->setIcon(IconLoader::Load("document-new"));
    m_actionSavePlaylist->setIcon(IconLoader::Load("document-save"));
    m_actionLoadPlaylist->setIcon(IconLoader::Load("document-open"));

    m_playlistContainer->SetActions(m_actionNewPlaylist,
            m_actionSavePlaylist,
            m_actionLoadPlaylist,
            NULL, NULL);

    connect(m_playlistContainer->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(slotPlayIndex(QModelIndex)));



    // Playlists Context menu Setup
    m_playlistMenu = new QMenu(this);

    m_playlistMenu->addAction(m_pAddToAutoDJAction);
    m_playlistMenu->addAction(m_pAddToAutoDJTopAction);
    m_playlistMenu->addSeparator();

    // Load to Deck Actions
    for (int i = 1; i <= iNumDecks; ++i) {
        QString deckGroup = QString("[Channel%1]").arg(i);
        QAction* pAction = (QAction*)m_groupMapper.mapping(deckGroup);
        if (pAction) {
            m_playlistMenu->addAction(pAction);
        }
    }

    m_playlistMenu->addAction(m_pMenuSampler->menuAction());
    m_playlistMenu->addSeparator();

    m_playlistMenu->addAction(m_pMenuCrate->menuAction());
    m_playlistMenu->addAction(m_pMenuPlaylist->menuAction());

    // m_playlistMenu->addAction(ui_->action_remove_from_playlist);

    m_playlistUndoRedo = m_playlistMenu->addSeparator();

    /*

    playlist_menu_->addAction(ui_->action_edit_track);
    playlist_menu_->addAction(ui_->action_edit_value);
    playlist_menu_->addAction(ui_->action_renumber_tracks);
    playlist_menu_->addAction(ui_->action_selection_set_value);
    playlist_menu_->addAction(ui_->action_auto_complete_tags);
    playlist_menu_->addSeparator();
    */

    /*
    playlist_copy_to_library_ = playlist_menu_->addAction(IconLoader::Load("edit-copy"), tr("Copy to library..."), this, SLOT(PlaylistCopyToLibrary()));
    playlist_move_to_library_ = playlist_menu_->addAction(IconLoader::Load("go-jump"), tr("Move to library..."), this, SLOT(PlaylistMoveToLibrary()));
    playlist_organise_ = playlist_menu_->addAction(IconLoader::Load("edit-copy"), tr("Organise files..."), this, SLOT(PlaylistMoveToLibrary()));
    */

    /*
    playlist_delete_ = playlist_menu_->addAction(IconLoader::Load("edit-delete"), tr("Delete from disk..."), this, SLOT(PlaylistDelete()));
    playlist_open_in_browser_ = playlist_menu_->addAction(IconLoader::Load("document-open-folder"), tr("Show in file browser..."), this, SLOT(PlaylistOpenInBrowser()));
    */

    /*
    playlist_menu_->addSeparator();
    playlist_menu_->addAction(ui_->action_clear_playlist);
    playlist_menu_->addAction(ui_->action_shuffle);
    */

    connect(m_playlistContainer->view(), SIGNAL(RightClicked(QPoint,QModelIndex)), SLOT(slotPlaylistRightClick(QPoint,QModelIndex)));

    connect(m_playlistContainer, SIGNAL(UndoRedoActionsChanged(QAction*,QAction*)),
            SLOT(slotPlaylistUndoRedoChanged(QAction*,QAction*)));


/*


    connect(m_playlistContainer->view(), SIGNAL(PlayItem(QModelIndex)), SLOT(PlayIndex(QModelIndex)));
    connect(m_playlistContainer->view(), SIGNAL(PlayPause()), player_, SLOT(PlayPause()));
    connect(m_playlistContainer->view(), SIGNAL(RightClicked(QPoint,QModelIndex)), SLOT(PlaylistRightClick(QPoint,QModelIndex)));
    connect(m_playlistContainer->view(), SIGNAL(SeekTrack(int)), ui_->track_slider, SLOT(Seek(int)));
    connect(m_playlistContainer->view(), SIGNAL(BackgroundPropertyChanged()), SLOT(RefreshStyleSheet()));
    */



    //m_playlistContainer->SetPlayer(player);
    //connect(player_, SIGNAL(Paused()), ui_->playlist->view(), SLOT(StopGlowing()));
    //connect(player_, SIGNAL(Playing()), ui_->playlist->view(), SLOT(StartGlowing()));
    //connect(player_, SIGNAL(Stopped()), ui_->playlist->view(), SLOT(StopGlowing()));
    //connect(player_, SIGNAL(Paused()), ui_->playlist, SLOT(ActivePaused()));
    //connect(player_, SIGNAL(Playing()), ui_->playlist, SLOT(ActivePlaying()));
    //connect(player_, SIGNAL(Stopped()), ui_->playlist, SLOT(ActiveStopped()));

    //connect(ui_->action_jump, SIGNAL(triggered()), ui_->playlist->view(), SLOT(JumpToCurrentlyPlayingTrack()));
    //connect(m_playlistContainer, SIGNAL(ViewSelectionModelChanged()), SLOT(PlaylistViewSelectionModelChanged()));
}


void ClementineView::setupLibraryFilerWidget() {

    // Library filter widget
    //QActionGroup* library_view_group = new QActionGroup(this);

    //library_show_all_ = library_view_group->addAction(tr("Show all songs"));
    //library_show_duplicates_ = library_view_group->addAction(tr("Show only duplicates"));
    //library_show_untagged_ = library_view_group->addAction(tr("Show only untagged"));

    //library_show_all_->setCheckable(true);
    //library_show_duplicates_->setCheckable(true);
    //library_show_untagged_->setCheckable(true);
    //library_show_all_->setChecked(true);

    //connect(library_view_group, SIGNAL(triggered(QAction*)), SLOT(ChangeLibraryQueryMode(QAction*)));

    //QAction* library_config_action = new QAction(
    //    IconLoader::Load("configure"), tr("Configure library..."), this);
    //connect(library_config_action, SIGNAL(triggered()), SLOT(ShowLibraryConfig()));
    //m_libraryViewContainer->filter()->SetSettingsGroup(kSettingsGroup);
    //m_libraryViewContainer->filter()->SetLibraryModel(library_->model());

    //QAction* separator = new QAction(this);
    //separator->setSeparator(true);

    //m_libraryViewContainer->filter()->AddMenuAction(library_show_all_);
    //m_libraryViewContainer->filter()->AddMenuAction(library_show_duplicates_);
    //m_libraryViewContainer->filter()->AddMenuAction(library_show_untagged_);
    //m_libraryViewContainer->filter()->AddMenuAction(separator);
    //m_libraryViewContainer->filter()->AddMenuAction(library_config_action);

}

void ClementineView::addToMixxPlaylist(int iPlaylistId, bool bTop) {

    PlaylistDAO& playlistDao = m_pTrackCollection->getPlaylistDAO();

    if (iPlaylistId <= 0 || !m_pData) {
        return;
    }

    if (const SongMimeData* songData = qobject_cast<const SongMimeData*>(m_pData)) {
        foreach (Song song, songData->songs) {
            qDebug() << "-----" << song.title();
            TrackPointer pTrack = getTrack(song);
            if (pTrack) {
                int iTrackId = pTrack->getId();
                if (iTrackId != -1) {
                    if (bTop) {
                        // Load track to position two because position one is already loaded to the player
                        playlistDao.insertTrackIntoPlaylist(iTrackId, iPlaylistId, 2);
                    }
                    else {
                        playlistDao.appendTrackToPlaylist(iTrackId, iPlaylistId);
                    }
                }
            }
        }
    //} else if (const GeneratorMimeData* generator_data = qobject_cast<const GeneratorMimeData*>(data)) {
    //    InsertSmartPlaylist(generator_data->generator_, row, play_now, enqueue_now);
    //} else if (const PlaylistItemMimeData* item_data = qobject_cast<const PlaylistItemMimeData*>(data)) {
    //    InsertItems(item_data->items_, row, play_now, enqueue_now);
    } else if (m_pData->hasUrls()) {
        foreach (QUrl url, m_pData->urls()) {
            qDebug() << "-----" << url;
            TrackPointer pTrack = getTrack(url);
            int iTrackId = pTrack->getId();
            if (iTrackId != -1) {
                if (bTop) {
                    // Load track to position two because position one is already loaded to the player
                    playlistDao.insertTrackIntoPlaylist(iTrackId, iPlaylistId, 2);
                }
                else {
                    playlistDao.appendTrackToPlaylist(iTrackId, iPlaylistId);
                }
            }
        }
    }
    delete m_pData;
}

void ClementineView::slotSendToAutoDJ() {
    // append to auto DJ
    PlaylistDAO& playlistDao = m_pTrackCollection->getPlaylistDAO();
    addToMixxPlaylist(playlistDao.getPlaylistIdFromName(AUTODJ_TABLE), false); // bTop = false
}

void ClementineView::slotSendToAutoDJTop() {
    PlaylistDAO& playlistDao = m_pTrackCollection->getPlaylistDAO();
    addToMixxPlaylist(playlistDao.getPlaylistIdFromName(AUTODJ_TABLE), true); // bTop = true
}

void ClementineView::slotAddToMixxxPlaylist(int iPlaylistId) {
    addToMixxPlaylist(iPlaylistId, false); // bTop = false
}

/*
void ClementineView::slotImportAsMixxxPlaylist() {
    // qDebug() << "slotAddToAutoDJ() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(m_lastRightClickedIndex.internalPointer());
        QString playlistName = item->data().toString();
        QString playlistStId = item->dataPath().toString();
        int playlistID = playlistStId.toInt();
        qDebug() << "BansheeFeature::slotImportAsMixxxPlaylist " << playlistName << " " << playlistStId;
        if (playlistID > 0) {
            BansheePlaylistModel* pPlaylistModelToAdd = new BansheePlaylistModel(this, m_pTrackCollection, &m_connection);
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
/*
        }
    }
}
*/

void ClementineView::loadSelectionToGroup(QString group) {
    // If the track load override is disabled, check to see if a track is
    // playing before trying to load it
    if (!m_pData) {
        return;
    }

    if ( !(m_pConfig->getValueString(ConfigKey("[Controls]","AllowTrackLoadToPlayingDeck")).toInt()) ) {
        bool groupPlaying = (ControlObject::getControl(
                ConfigKey(group, "play"))->get() == 1.0f);

        if (groupPlaying) {
            return;
        }
    }

    TrackPointer pTrack;

    if (const SongMimeData* songData = qobject_cast<const SongMimeData*>(m_pData)) {
        foreach (Song song, songData->songs) {
            qDebug() << "-----" << song.title();
            pTrack = getTrack(song);
            break;
        }
    } else if (m_pData->hasUrls()) {
        foreach (QUrl url, m_pData->urls()) {
            qDebug() << "-----" << url;
            pTrack = getTrack(url);
            break;
        }
    }

    if (pTrack) {
        emit(loadTrackToPlayer(pTrack, group));
    }

    delete m_pData;
}

void ClementineView::slotPlayIndex(const QModelIndex& index) {
    if (!index.isValid())
        return;

    int row = index.row();
    if (index.model() == m_playlistsManager->current()->proxy()) {
        // The index was in the proxy model (might've been filtered), so we need
        // to get the actual row in the source model.
        row = m_playlistsManager->current()->proxy()->mapToSource(index).row();
    }

    m_playlistsManager->SetActiveToCurrent();
    m_playlistsManager->active()->set_current_row(-1);
    m_playlistsManager->active()->set_current_row(row);

    if (m_playlistsManager->active()->current_row() == -1) {
        // Maybe index didn't exist in the playlist.
        return;
    }

    PlaylistItemPtr current_item = m_playlistsManager->active()->current_item();
    // const QUrl url = current_item->Url();
    TrackPointer pTrack = getTrack(current_item->Metadata());

    if (pTrack) {
        emit(loadTrack(pTrack));
    }
}


void ClementineView::slotLibraryViewRightClicked(QMimeData* data) {
    // setup Context Menu actions
    qDebug() << "slotLibraryViewRightClicked";

    m_pData = data;

    bool oneSongSelected = false;
    if (data) {
        if (data->hasUrls()) {
            if (data->urls().count() == 1) {
                oneSongSelected = true;
            }
        }
    }

    ControlObjectThreadMain* pNumDecks = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Master]", "num_decks")));
    int iNumDecks = pNumDecks->get();
    delete pNumDecks;

    // Load to Deck Actions
    for (int i = 1; i <= iNumDecks; ++i) {
        QString deckGroup = QString("[Channel%1]").arg(i);
        QAction* pAction = (QAction*)m_groupMapper.mapping(deckGroup);
        if (pAction) {
            bool deckPlaying = ControlObject::getControl(
                    ConfigKey(deckGroup, "play"))->get() == 1.0f;
            pAction->setEnabled(!deckPlaying && oneSongSelected);
        }
    }

    ControlObjectThreadMain* pNumSamplers = new ControlObjectThreadMain(
            ControlObject::getControl(ConfigKey("[Master]", "num_samplers")));
    int iNumSamplers = pNumSamplers->get();
    delete pNumSamplers;

    for (int i = 1; i <= iNumSamplers; ++i) {
        QString samplerGroup = QString("[Sampler%1]").arg(i);
        QAction* pAction = (QAction*)m_groupMapper.mapping(samplerGroup);
        if (pAction) {
            bool samplerPlaying = ControlObject::getControl(
                    ConfigKey(samplerGroup, "play"))->get() == 1.0f;
            pAction->setEnabled(!samplerPlaying && oneSongSelected);
        }
    }

    m_pMenuSampler->setEnabled(oneSongSelected);

    m_pMenuPlaylist->clear();
    PlaylistDAO& playlistDao = m_pTrackCollection->getPlaylistDAO();
    int numPlaylists = playlistDao.playlistCount();

    if (numPlaylists) {
        for (int i = 0; i < numPlaylists; ++i) {
            int iPlaylistId = playlistDao.getPlaylistId(i);

            if (!playlistDao.isHidden(iPlaylistId)) {
                QAction* pAction = new QAction(playlistDao.getPlaylistName(iPlaylistId), m_pMenuPlaylist);
                bool locked = playlistDao.isPlaylistLocked(iPlaylistId);
                pAction->setEnabled(!locked);
                m_pMenuPlaylist->addAction(pAction);
                m_playlistMapper.setMapping(pAction, iPlaylistId);
                connect(pAction, SIGNAL(triggered()), &m_playlistMapper, SLOT(map()));
            }
        }
    } else {
        m_pMenuPlaylist->setVisible(false);
        m_pMenuPlaylist->menuAction()->setEnabled(false);
    }

    m_pMenuCrate->clear();
    CrateDAO& crateDao = m_pTrackCollection->getCrateDAO();
    int numCrates = crateDao.crateCount();

    if (numCrates) {
        for (int i = 0; i < numCrates; ++i) {
            int iCrateId = crateDao.getCrateId(i);
            QAction* pAction = new QAction(crateDao.crateName(iCrateId), m_pMenuCrate);
            pAction->setEnabled(!crateDao.isCrateLocked(iCrateId));
            m_pMenuCrate->addAction(pAction);
            m_crateMapper.setMapping(pAction, iCrateId);
            connect(pAction, SIGNAL(triggered()), &m_crateMapper, SLOT(map()));
        }
    } else {
        m_pMenuCrate->setVisible(false);
        m_pMenuCrate->menuAction()->setEnabled(false);
    }
}

void ClementineView::slotAddToClementinePlaylist(QMimeData* data) {
    if (!data) {
        return;
    }

    if (const SongMimeData* songData = qobject_cast<const SongMimeData*>(data)) {
        // Should we replace the flags with the user's preference?
        if (songData->override_user_settings_) {
            // Do nothing
        } else if (songData->from_doubleclick_) {
            TrackPointer pTrack;
            foreach (Song song, songData->songs) {
                qDebug() << "-----" << song.title();
                pTrack = getTrack(song);
                break;
            }
            if (pTrack) {
                emit(loadTrack(pTrack));
            }
            //ApplyAddBehaviour(doubleclick_addmode_, mime_data);
            //ApplyPlayBehaviour(doubleclick_playmode_, mime_data);
        } else {
            //ApplyPlayBehaviour(menu_playmode_, mime_data);
        }

        // Should we create a new playlist for the songs?
        //if(mime_data->open_in_new_playlist_) {
            //playlists_->New(mime_data->get_name_for_new_playlist());
        //}
    }

    //playlists_->current()->dropMimeData(data, Qt::CopyAction, -1, 0, QModelIndex());
    delete data;
}


void ClementineView::addSelectionToCrate(int iCrateId) {
    CrateDAO& crateDao = m_pTrackCollection->getCrateDAO();

    if (iCrateId <= 0 || !m_pData) {
        return;
    }

    if (const SongMimeData* songData = qobject_cast<const SongMimeData*>(m_pData)) {
        foreach (Song song, songData->songs) {
            qDebug() << "-----" << song.title();
            TrackPointer pTrack = getTrack(song);
            if (pTrack) {
                int iTrackId = pTrack->getId();
                if (iTrackId != -1) {
                    crateDao.addTrackToCrate(iTrackId, iCrateId);
                }
            }
        }
    //} else if (const GeneratorMimeData* generator_data = qobject_cast<const GeneratorMimeData*>(data)) {
    //    InsertSmartPlaylist(generator_data->generator_, row, play_now, enqueue_now);
    //} else if (const PlaylistItemMimeData* item_data = qobject_cast<const PlaylistItemMimeData*>(data)) {
    //    InsertItems(item_data->items_, row, play_now, enqueue_now);
    } else if (m_pData->hasUrls()) {
        foreach (QUrl url, m_pData->urls()) {
            qDebug() << "-----" << url;
            TrackPointer pTrack = getTrack(url);
            if (pTrack) {
                int iTrackId = pTrack->getId();
                if (iTrackId != -1) {
                    crateDao.addTrackToCrate(iTrackId, iCrateId);
                }
            }
        }
    }
    delete m_pData;
}

TrackPointer ClementineView::getTrack(const Song& song) {
    QString location;

    location = song.url().toLocalFile();

    TrackPointer pTrack = TrackPointer();

    if (location.isEmpty()) {
        return pTrack;
    }

    TrackDAO& track_dao = m_pTrackCollection->getTrackDAO();
    int track_id = track_dao.getTrackId(location);
    bool track_already_in_library = (track_id >= 0);
    if (track_id < 0) {
        // Add Track to library
        track_id = track_dao.addTrack(location, true);
    }

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
        pTrack->setArtist(song.artist());
        pTrack->setTitle(song.title());
        pTrack->setDuration(song.length_nanosec());
        pTrack->setAlbum(song.album());
        pTrack->setYear(QString::number(song.year()));
        pTrack->setRating((int)song.rating());
        pTrack->setGenre(song.genre());
        pTrack->setTrackNumber(QString::number(song.track()));
        double bpm = (double)song.bpm();
        pTrack->setBpm(bpm);
        pTrack->setBitrate(song.bitrate());
        pTrack->setComment(song.comment());
        pTrack->setComposer(song.composer());
        // If the track has a BPM, then give it a static beatgrid.
        if (bpm) {
            BeatsPointer pBeats = BeatFactory::makeBeatGrid(pTrack.data(), bpm, 0.0f);
            pTrack->setBeats(pBeats);
        }
    }
    return pTrack;
}

TrackPointer ClementineView::getTrack(const QUrl& url) {
    QString location;

    location = url.toLocalFile();

    if (location.isEmpty()) {
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
        // Add Track to library failed, create a transient TrackInfoObject
        pTrack = TrackPointer(new TrackInfoObject(location), &QObject::deleteLater);
    } else {
        pTrack = track_dao.getTrack(track_id);
    }
    return pTrack;
}

void ClementineView::slotPlaylistUndoRedoChanged(QAction *undo, QAction *redo) {
  m_playlistMenu->insertAction(m_playlistUndoRedo, undo);
  m_playlistMenu->insertAction(m_playlistUndoRedo, redo);
}

void ClementineView::slotPlaylistRightClick(const QPoint& global_pos, const QModelIndex& index) {
    QModelIndex source_index =
            m_playlistsManager->current()->proxy()->mapToSource(index);

    m_playlistMenuIndex = source_index;

    // Are any of the selected songs editable or queued?
    QModelIndexList selection =
            m_playlistContainer->view()->selectionModel()->selection().indexes();

    QMimeData* data = m_playlistsManager->current()->proxy()->mimeData(selection);
    slotLibraryViewRightClicked(data);

    bool cue_selected = false;
    int editable = 0;
    int streams = 0;
    int in_queue = 0;
    int not_in_queue = 0;
    foreach (const QModelIndex& index, selection) {
        if (index.column() != 0) {
            continue;
        }

        PlaylistItemPtr item = m_playlistsManager->current()->item_at(index.row());
        if (item->Metadata().has_cue()) {
            cue_selected = true;
        } else if (item->Metadata().IsEditable()) {
            editable++;
        }

        if (item->Metadata().is_stream()) {
            streams++;
        }

        if (index.data(Playlist::Role_QueuePosition).toInt() == -1) {
            not_in_queue++;
        } else {
            in_queue++;
        }
    }

    int all = not_in_queue + in_queue;

    // this is available when we have one or many files and at least one of
    // those is not CUE related
    //ui_->action_edit_track->setEnabled(editable);
    //ui_->action_edit_track->setVisible(editable);
    //ui_->action_auto_complete_tags->setEnabled(editable);
    //ui_->action_auto_complete_tags->setVisible(editable);
    // the rest of the read / write actions work only when there are no CUEs
    // involved
    if (cue_selected) {
        editable = 0;
    }

    // no 'show in browser' action if only streams are selected
    //playlist_open_in_browser_->setVisible(streams != all);

    bool track_column = (index.column() == Playlist::Column_Track);
    //ui_->action_renumber_tracks->setVisible(editable >= 2 && track_column);
    //ui_->action_selection_set_value->setVisible(editable >= 2 && !track_column);
    //ui_->action_edit_value->setVisible(editable);
    //ui_->action_remove_from_playlist->setEnabled(!selection.isEmpty());

    //playlist_copy_to_library_->setVisible(false);
    //playlist_move_to_library_->setVisible(false);
    //playlist_organise_->setVisible(false);
    //playlist_delete_->setVisible(false);
    //playlist_copy_to_device_->setVisible(false);

    /*
    if (!index.isValid()) {
        ui_->action_selection_set_value->setVisible(false);
        ui_->action_edit_value->setVisible(false);
    } else {
        Playlist::Column column = (Playlist::Column) index.column();
        bool column_is_editable = Playlist::column_is_editable(column)
                && editable;

        ui_->action_selection_set_value->setVisible(
                ui_->action_selection_set_value->isVisible()
                        && column_is_editable);
        ui_->action_edit_value->setVisible(
                ui_->action_edit_value->isVisible() && column_is_editable);

        QString column_name = Playlist::column_name(column);
        QString column_value =
                playlists_->current()->data(source_index).toString();
        if (column_value.length() > 25)
            column_value = column_value.left(25) + "...";

        ui_->action_selection_set_value->setText(
                tr("Set %1 to \"%2\"...").arg(column_name.toLower()).arg(
                        column_value));
        ui_->action_edit_value->setText(
                tr("Edit tag \"%1\"...").arg(column_name));

        // Is it a library item?
        PlaylistItemPtr item = playlists_->current()->item_at(
                source_index.row());
        if (item->IsLocalLibraryItem() && item->Metadata().id() != -1) {
            playlist_organise_->setVisible(editable);
        } else {
            playlist_copy_to_library_->setVisible(editable);
            playlist_move_to_library_->setVisible(editable);
        }

        playlist_delete_->setVisible(editable);
        playlist_copy_to_device_->setVisible(editable);

        // Remove old item actions, if any.
        foreach(QAction* action, playlistitem_actions_) {
            playlist_menu_->removeAction(action);
        }

        // Get the new item actions, and add them
        playlistitem_actions_ = item->actions();
        playlist_menu_->insertActions(ui_->action_clear_playlist,
                playlistitem_actions_);
    }
    */

    // if it isn't the first time we right click, we need to remove the menu previously created
    if (m_playlistAddToAnother != NULL) {
        m_playlistMenu->removeAction(m_playlistAddToAnother);
        delete m_playlistAddToAnother;
    }

    // create the playlist submenu
    QMenu* add_to_another_menu = new QMenu(tr("Add to another Clementine playlist"), this);
    add_to_another_menu->setIcon((IconLoader::Load("add")));

    PlaylistBackend::Playlist playlist;
    foreach (playlist, m_pClementineApp->playlist_backend()->GetAllPlaylists()) {
        //don't add the current playlist
        if (playlist.id != m_playlistsManager->current()->id()) {
            QAction* existing_playlist = new QAction(this);
            existing_playlist->setText(playlist.name);
            existing_playlist->setData(playlist.id);
            add_to_another_menu->addAction(existing_playlist);
        }
    }
    add_to_another_menu->addSeparator();
   //add to a new playlist
    QAction* new_playlist = new QAction(this);
    new_playlist->setText(tr("New playlist"));
    new_playlist->setData(-1); //fake id
    add_to_another_menu->addAction(new_playlist);
    //m_playlistAddToAnother = m_playlistMenu->insertMenu(
    //        ui_->action_remove_from_playlist, add_to_another_menu);
    m_playlistAddToAnother = m_playlistMenu->addMenu(add_to_another_menu);

    connect(add_to_another_menu, SIGNAL(triggered(QAction*)),
            SLOT(slotAddToClementinePlaylist(QAction*)));

    m_playlistMenu->popup(global_pos);
}

void ClementineView::slotAddToClementinePlaylist(QAction* action) {

    int playlist_id = action->data().toInt();
    PlaylistItemList items;

    if (!m_pData) {
        return;
    }

    const SongMimeData* songData = qobject_cast<const SongMimeData*>(m_pData);

    if (songData) {
        //we're creating a new playlist
        if (playlist_id == -1) {
            //save the current playlist to reactivate it
            int current_id = m_playlistsManager->current_id();
            //get the name from selection
            m_playlistsManager->New(m_playlistsManager->GetNameForNewPlaylist(songData->songs));
            if (m_playlistsManager->current()->id() != current_id) {
                //I'm sure the new playlist was created and is selected, so I can just insert items
                m_playlistsManager->current()->InsertSongs(songData->songs);
                //set back the current playlist
                m_playlistsManager->SetCurrentPlaylist(current_id);
            }
        } else {
            //we're inserting in a existing playlist
            m_playlistsManager->playlist(playlist_id)->InsertSongs(songData->songs);
        }
    }
}

