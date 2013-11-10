/*
 * ClementineView.h
 *
 *  Created on: 25.02.2012
 *      Author: daniel
 */

#ifndef CLEMENTINEVIEW_H_
#define CLEMENTINEVIEW_H_

#include <QtCore/qobject.h>

#include "configobject.h"
#include <trackinfoobject.h>

#include <QSignalMapper>
#include <QModelIndex>

#include "library/abstractlibraryview.h"

// From clementine-player
#include "library/libraryviewcontainer.h"
#include "playlist/playlistcontainer.h"

class QSortFilterProxyModel;
class LibraryViewContainer;
class Library;
class TaskManager;
class QMimeData;
class ClementineFeature;
class TrackCollection;
class Song;
class QUrl;
class QMenu;
class QHBoxLayout;
class PlaylistBackend;
class PlaylistSequence;
class Database;
class Application;


class ClementineView: public QWidget, public virtual AbstractLibraryView {
    Q_OBJECT
public:
    ClementineView(QWidget* parent, ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection);
    virtual ~ClementineView();

    void setup(QDomNode node) {};
    void onSearchStarting() {};
    void onSearchCleared() {};
    void onSearch(const QString& text) {};
    void onShow() {};

    // If applicable, requests that the LibraryView load the selected
    // track. Does nothing otherwise.
    void loadSelectedTrack() {};

    // If a selePlaylistSequencection is applicable for this view, request that the selection be
    // increased or decreased by the provided delta. For example, for a value of
    // 1, the view should move to the next selection in the list.
    void moveSelection(int delta) {};

    void setApplication(Application* app);

  signals:
    void loadTrack(TrackPointer tio);
    void loadTrackToPlayer(TrackPointer tio, QString group);

  public slots:
    void slotLibraryViewRightClicked(QMimeData* data);
    void slotSendToAutoDJ();
    void slotSendToAutoDJTop();
    void slotAddToMixxxPlaylist(int iPlaylistId);
    void slotAddToClementinePlaylist(QMimeData* data);
    void loadSelectionToGroup(QString group);
    void addSelectionToCrate(int iCrateId);
    void slotPlayIndex(const QModelIndex& index);
    void slotPlaylistUndoRedoChanged(QAction*,QAction*);
    void slotPlaylistRightClick(const QPoint& global_pos, const QModelIndex& index);
    void slotAddToClementinePlaylist(QAction* action);


private:
    void setupLibraryFilerWidget();
    void addToMixxPlaylist(int iPlaylistId, bool bTop);
    TrackPointer getTrack(const Song& song);
    TrackPointer getTrack(const QUrl& url);

    Application* m_pClementineApp;

    LibraryViewContainer* m_libraryViewContainer;
    PlaylistContainer* m_playlistContainer;
    PlaylistManager* m_playlistsManager;
    PlaylistSequence* m_playlistSequence;

    QSortFilterProxyModel* m_librarySortModel;

    ConfigObject<ConfigValue>* m_pConfig;
    TrackCollection* m_pTrackCollection;

    QMimeData* m_pData;
    QModelIndex m_playlistMenuIndex;

    QSignalMapper m_playlistMapper;
    QSignalMapper m_crateMapper;
    QSignalMapper m_groupMapper;
    QSignalMapper m_samplerMapper;

    QAction* m_pAddToAutoDJAction;
    QAction* m_pAddToAutoDJTopAction;
    QMenu* m_pMenuSampler;
    QMenu* m_pMenuCrate;
    QMenu* m_pMenuPlaylist;

    QHBoxLayout* m_horizontalLayout;

    QAction* m_actionNewPlaylist;
    QAction* m_actionSavePlaylist;
    QAction* m_actionLoadPlaylist;

    QMenu* m_playlistMenu;
    QAction* m_playlistUndoRedo;
    QAction* m_playlistAddToAnother;



    /*
    QAction* playlist_organise_;
    QAction* playlist_copy_to_library_;
    QAction* playlist_move_to_library_;
    QAction* playlist_copy_to_device_;
    QAction* playlist_delete_;
    QAction* playlist_open_in_browser_;
    QAction* playlist_queue_;

    QList<QAction*> playlistitem_actions_;
    QModelIndex playlist_menu_index_;
    */

    static const char* kSettingsGroup;

};

#endif /* CLEMENTINEVIEW_H_ */
