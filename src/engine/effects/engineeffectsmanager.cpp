#include "engine/effects/engineeffectsmanager.h"

#include "engine/effects/engineeffectchain.h"
#include "engine/effects/engineeffect.h"
#include "util/timer.h"

EngineEffectsManager::EngineEffectsManager(EffectsResponsePipe* pResponsePipe)
        : m_pResponsePipe(pResponsePipe) {
}

EngineEffectsManager::~EngineEffectsManager() {
}

void EngineEffectsManager::onCallbackStart() {
    EffectsRequest* request = NULL;
    while (m_pResponsePipe->readMessages(&request, 1) > 0) {
        EffectsResponse response(*request);
        bool processed = false;
        switch (request->type) {
            case EffectsRequest::ADD_EFFECT_CHAIN:
            case EffectsRequest::REMOVE_EFFECT_CHAIN:
                if (processEffectsRequest(*request, m_pResponsePipe.data())) {
                    processed = true;
                }
                break;
            case EffectsRequest::ADD_EFFECT_TO_CHAIN:
            case EffectsRequest::REMOVE_EFFECT_FROM_CHAIN:
            case EffectsRequest::SET_EFFECT_CHAIN_PARAMETERS:
            case EffectsRequest::ENABLE_EFFECT_CHAIN_FOR_GROUP:
            case EffectsRequest::DISABLE_EFFECT_CHAIN_FOR_GROUP:
                if (!m_chains.contains(request->pTargetChain)) {
                    qDebug() << debugString()
                             << "WARNING: message for unloaded chain"
                             << request->pTargetChain;
                    response.success = false;
                    response.status = EffectsResponse::NO_SUCH_CHAIN;
                } else {
                    processed = request->pTargetChain->processEffectsRequest(
                        *request, m_pResponsePipe.data());

                    if (processed) {
                        // When an effect becomes active (part of a chain), keep
                        // it in our master list so that we can respond to
                        // requests about it.
                        if (request->type == EffectsRequest::ADD_EFFECT_TO_CHAIN) {
                            m_effects.append(request->AddEffectToChain.pEffect);
                        } else if (request->type == EffectsRequest::REMOVE_EFFECT_FROM_CHAIN) {
                            m_effects.removeAll(request->RemoveEffectFromChain.pEffect);
                        }
                    } else {
                        if (!processed) {
                            // If we got here, the message was not handled for
                            // an unknown reason.
                            response.success = false;
                            response.status = EffectsResponse::INVALID_REQUEST;
                        }
                    }
                }
                break;
            case EffectsRequest::SET_EFFECT_PARAMETER:
                if (!m_effects.contains(request->pTargetEffect)) {
                    qDebug() << debugString()
                             << "WARNING: SetEffectParameter message for unloaded effect"
                             << request->pTargetEffect;
                    response.success = false;
                    response.status = EffectsResponse::NO_SUCH_EFFECT;
                } else {
                    processed = request->pTargetEffect
                            ->processEffectsRequest(*request, m_pResponsePipe.data());

                    if (!processed) {
                        // If we got here, the message was not handled for an
                        // unknown reason.
                        response.success = false;
                        response.status = EffectsResponse::INVALID_REQUEST;
                    }
                }
                break;
            default:
                response.success = false;
                response.status = EffectsResponse::UNHANDLED_MESSAGE_TYPE;
                break;
        }

        if (!processed) {
            m_pResponsePipe->writeMessages(&response, 1);
        }
    }
}

static QList<int> list;

void EngineEffectsManager::process(const QString& group,
                                   const CSAMPLE* pInput, CSAMPLE* pOutput,
                                   const unsigned int numSamples) {
    ScopedTimer timer("EngineEffectsManager::process()");
    foreach (EngineEffectChain* pChain, m_chains) {
        pChain->process(group, pInput, pOutput, numSamples);
    }
}

bool EngineEffectsManager::addEffectChain(EngineEffectChain* pChain) {
    if (m_chains.contains(pChain)) {
        qDebug() << debugString() << "WARNING: EffectChain already added to EngineEffectsManager:"
                 << pChain->id();
        return false;
    }
    m_chains.append(pChain);
    return true;
}

bool EngineEffectsManager::removeEffectChain(EngineEffectChain* pChain) {
    return m_chains.removeAll(pChain) > 0;
}

bool EngineEffectsManager::processEffectsRequest(const EffectsRequest& message,
                                                 EffectsResponsePipe* pResponsePipe) {
    EffectsResponse response(message);
    switch (message.type) {
        case EffectsRequest::ADD_EFFECT_CHAIN:
            qDebug() << debugString() << "ADD_EFFECT_CHAIN"
                     << message.AddEffectChain.pChain;
            response.success = addEffectChain(message.AddEffectChain.pChain);
            break;
        case EffectsRequest::REMOVE_EFFECT_CHAIN:
            qDebug() << debugString() << "REMOVE_EFFECT_CHAIN"
                     << message.RemoveEffectChain.pChain;
            response.success = removeEffectChain(message.RemoveEffectChain.pChain);
            break;
        default:
            return false;
    }
    pResponsePipe->writeMessages(&response, 1);
    return true;
}
