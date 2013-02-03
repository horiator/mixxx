// playermanager.h
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <QList>

#include "configobject.h"
#include "trackinfoobject.h"

class ControlObject;
class Deck;
class Sampler;
class PreviewDeck;
class BaseTrackPlayer;

class LibraryFeatures;
class EngineMaster;
class AnalyserQueue;
class SoundManager;
class VinylControlManager;
class TrackCollection;

class PlayerManager : public QObject {
    Q_OBJECT
  public:
    PlayerManager(ConfigObject<ConfigValue>* pConfig,
                  SoundManager* pSoundManager,
                  EngineMaster* pEngine,
                  VinylControlManager* pVCManager);
    virtual ~PlayerManager();

    // Add a deck to the PlayerManager
    Deck* addDeck();

    // Add a sampler to the PlayerManager
    Sampler* addSampler();

    // Add a PreviewDeck to the PlayerManager
    PreviewDeck* addPreviewDeck();

    // Return the number of players. Thread-safe.
    static unsigned int numDecks();

    // Return the number of samplers. Thread-safe.
    static unsigned int numSamplers();

    // Return the number of preview decks. Thread-safe.
    static unsigned int numPreviewDecks();

    // Get a BaseTrackPlayer (i.e. a Deck or a Sampler) by its group
    BaseTrackPlayer* getPlayer(QString group) const;

    // Get the deck by its deck number. Decks are numbered starting with 1.
    Deck* getDeck(unsigned int player) const;

    PreviewDeck* getPreviewDeck(unsigned int libPreviewPlayer) const;

    // Get the sampler by its number. Samplers are numbered starting with 1.
    Sampler* getSampler(unsigned int sampler) const;

    // Binds signals between PlayerManager and LibraryFeatures. Does not store a pointer
    // to the Library.
    void bindToLibrary(LibraryFeatures* pLibrary);

    // Returns the group for the ith sampler where i is zero indexed
    static QString groupForSampler(int i) {
        return QString("[Sampler%1]").arg(i+1);
    }

    // Returns the group for the ith deck where i is zero indexed
    static QString groupForDeck(int i) {
        return QString("[Channel%1]").arg(i+1);
    }

    // Returns the group for the ith PreviewDeck where i is zero indexed
    static QString groupForPreviewDeck(int i) {
        return QString("[PreviewDeck%1]").arg(i+1);
    }

  public slots:
    // Slots for loading tracks into a Player, which is either a Sampler or a Deck
    void slotLoadTrackToPlayer(TrackPointer pTrack, QString group, bool play = false);
    void slotLoadToPlayer(QString location, QString group);

    // Slots for loading tracks to decks
    void slotLoadTrackIntoNextAvailableDeck(TrackPointer pTrack);
    // Loads the location to the deck. deckNumber is 1-indexed
    void slotLoadToDeck(QString location, int deckNumber);

    // Loads the location to the preview deck. previewDeckNumber is 1-indexed
    void slotLoadToPreviewDeck(QString location, int previewDeckNumber);
    // Slots for loading tracks to samplers
    void slotLoadTrackIntoNextAvailableSampler(TrackPointer pTrack);
    // Loads the location to the sampler. samplerNumber is 1-indexed
    void slotLoadToSampler(QString location, int samplerNumber);

    void slotNumDecksControlChanged(double v);
    void slotNumSamplersControlChanged(double v);
    void slotNumPreviewDecksControlChanged(double v);

  signals:
    void loadLocationToPlayer(QString location, QString group);

  private:
    TrackPointer lookupTrack(QString location);
    ConfigObject<ConfigValue>* m_pConfig;
    SoundManager* m_pSoundManager;
    EngineMaster* m_pEngine;
    VinylControlManager* m_pVCManager;
    AnalyserQueue* m_pAnalyserQueue;
    ControlObject* m_pCONumDecks;
    ControlObject* m_pCONumSamplers;
    ControlObject* m_pCONumPreviewDecks;

    QList<Deck*> m_decks;
    QList<Sampler*> m_samplers;
    QList<PreviewDeck*> m_preview_decks;
    QMap<QString, BaseTrackPlayer*> m_players;
};

#endif // PLAYERMANAGER_H
