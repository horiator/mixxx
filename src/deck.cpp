#include "deck.h"

Deck::Deck(QObject* pParent,
           ConfigObject<ConfigValue>* pConfig,
           EngineMaster* pMixingEngine,
           EffectsManager* pEffectsManager,
           EngineChannel::ChannelOrientation defaultOrientation,
           const StringAtom& group) :
        BaseTrackPlayerImpl(pParent, pConfig, pMixingEngine, pEffectsManager,
                            defaultOrientation, group, true, false) {
}

Deck::~Deck() {
}
