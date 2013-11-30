#ifndef ENGINEEFFECT_H
#define ENGINEEFFECT_H

#include <QMap>
#include <QString>
#include <QList>
#include <QVector>
#include <QtDebug>

#include "effects/effectmanifest.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/effects/message.h"

class EngineEffect : public EffectsRequestHandler {
  public:
    EngineEffect(const EffectManifest& manifest, EffectProcessor* pProcessor);
    virtual ~EngineEffect();

    const QString& name() const {
        return m_manifest.name();
    }

    EngineEffectParameter* getParameterById(const QString& id) {
        return m_parametersById.value(id, NULL);
    }

    void setParameterById(const QString& id, const QVariant& value) {
        EngineEffectParameter* pParameter = getParameterById(id);
        if (pParameter) {
            pParameter->setValue(value);
        }
    }

    bool processEffectsRequest(
        const EffectsRequest& message,
        EffectsResponsePipe* pResponsePipe);

  private:
    QString debugString() const {
        return QString("EngineEffect(%1)").arg(m_manifest.name());
    }

    EffectManifest m_manifest;
    EffectProcessor* m_pProcessor;
    // Must not be modified after construction.
    QVector<EngineEffectParameter*> m_parameters;
    QMap<QString, EngineEffectParameter*> m_parametersById;
};

#endif /* ENGINEEFFECT_H */