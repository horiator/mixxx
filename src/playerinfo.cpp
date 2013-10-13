/***************************************************************************
                      playerinfo.cpp  -  Helper class to have easy access
                                         to a lot of data (singleton)
                             -------------------
    copyright            : (C) 2007 by Wesley Stessens
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QMutexLocker>

#include "playerinfo.h"
#include "controlobject.h"
#include "controlobjectthread.h"
#include "engine/enginechannel.h"
#include "engine/enginexfader.h"
#include "playermanager.h"
#include "util/timer.h"

static const int kPlayingDeckUpdateIntervalMillis = 2000;

PlayerInfo::PlayerInfo()
        : m_currentlyPlayingDeck(-1) {
    startTimer(kPlayingDeckUpdateIntervalMillis);
}

PlayerInfo::~PlayerInfo() {
    m_loadedTrackMap.clear();
}

PlayerInfo &PlayerInfo::Instance() {
    static PlayerInfo playerInfo;
    return playerInfo;
}

TrackPointer PlayerInfo::getTrackInfo(const QString& group) {
    QMutexLocker locker(&m_mutex);
    return m_loadedTrackMap.value(group);
}

void PlayerInfo::setTrackInfo(const QString& group, const TrackPointer& track) {
    QMutexLocker locker(&m_mutex);
    TrackPointer pOld = m_loadedTrackMap.value(group);
    if (pOld) {
        emit(trackUnloaded(group, pOld));
    }
    m_loadedTrackMap.insert(group, track);
    emit(trackLoaded(group, track));
}

bool PlayerInfo::isTrackLoaded(const TrackPointer& pTrack) const {
    QMutexLocker locker(&m_mutex);
    QMapIterator<QString, TrackPointer> it(m_loadedTrackMap);
    while (it.hasNext()) {
        it.next();
        if (it.value() == pTrack) {
            return true;
        }
    }
    return false;
}

QMap<QString, TrackPointer> PlayerInfo::getLoadedTracks() {
    QMutexLocker locker(&m_mutex);
    QMap<QString, TrackPointer> ret = m_loadedTrackMap;
    return ret;
}

bool PlayerInfo::isFileLoaded(const QString& track_location) const {
    QMutexLocker locker(&m_mutex);
    QMapIterator<QString, TrackPointer> it(m_loadedTrackMap);
    while (it.hasNext()) {
        it.next();
        TrackPointer pTrack = it.value();
        if (pTrack) {
            if (pTrack->getLocation() == track_location) {
                return true;
            }
        }
    }
    return false;
}

void PlayerInfo::timerEvent(QTimerEvent* pTimerEvent) {
    Q_UNUSED(pTimerEvent);
    updateCurrentPlayingDeck();
}

void PlayerInfo::updateCurrentPlayingDeck() {
    QMutexLocker locker(&m_mutex);

    double maxVolume = 0;
    int maxDeck = -1;

    for (unsigned int i = 0; i < PlayerManager::numDecks(); ++i) {



        QString group = PlayerManager::groupForDeck(i);
        qDebug() << group;

        {
            ScopedTimer t("PlayerInfo::updateCurrentPlayingDeck() 1");
            // 1290ns
            // min=2135ns
            // min=2165ns
            double fvol1 = ControlObject::get(ConfigKey(group, "volume"));
        }
        {
            ScopedTimer t("PlayerInfo::updateCurrentPlayingDeck() 0");
            // 1825ns
            // min=2435ns
            // min=1930ns
            double fvol = getControlObjectThread(ConfigKey(group, "back"))->get();
        }

        ControlObjectThread* pCOT;
        {
            ScopedTimer t("PlayerInfo::updateCurrentPlayingDeck() 2");
            // 1270ns
            // min=1570ns
            // min=1160ns
            pCOT = getControlObjectThread(ConfigKey(group, "orientation"));
        }
        {
            ScopedTimer t("PlayerInfo::updateCurrentPlayingDeck() 3");
            // 645ns
            // min=830ns
            // min=695ns
            double fvol2 = pCOT->get();
        }

        if (getControlObjectThread(ConfigKey(group, "play"))->get() == 0.0) {
            continue;
        }

        if (getControlObjectThread(ConfigKey(group, "pregain"))->get() <= 0.5) {
            continue;
        }



//        if (fvol == 0.0) {
//            continue;
//        }

        double xfl, xfr;
        EngineXfader::getXfadeGains(
                getControlObjectThread(ConfigKey("[Master]", "crossfader"))->get(),
                1.0, 0.0, false, false, &xfl, &xfr);

        int orient = getControlObjectThread(ConfigKey(group, "orientation"))->get();
        double xfvol;
        if (orient == EngineChannel::LEFT) {
            xfvol = xfl;
        } else if (orient == EngineChannel::RIGHT) {
            xfvol = xfr;
        } else {
            xfvol = 1.0;
        }
/*
        double dvol = fvol * xfvol;
        if (dvol > maxVolume) {
            maxDeck = i;
            maxVolume = dvol;
        }
*/    }

    if (maxDeck != m_currentlyPlayingDeck) {
        m_currentlyPlayingDeck = maxDeck;
        locker.unlock();
        emit(currentPlayingDeckChanged(maxDeck));
    }
}

int PlayerInfo::getCurrentPlayingDeck() {
    QMutexLocker locker(&m_mutex);
    return m_currentlyPlayingDeck;
}

TrackPointer PlayerInfo::getCurrentPlayingTrack() {
    int deck = getCurrentPlayingDeck();
    if (deck >= 0) {
        return getTrackInfo(PlayerManager::groupForDeck(deck));
    }
    return TrackPointer();
}

ControlObjectThread* PlayerInfo::getControlObjectThread(const ConfigKey& key) {
    ControlObjectThread* cot = m_controlCache.value(key, NULL);
    if (cot == NULL) {
        // create COT
        cot = new ControlObjectThread(key, this);
        if (cot->valid()) {
            m_controlCache.insert(key, cot);
        } else {
            delete cot;
            cot = NULL;
        }
    }
    return cot;
}

void PlayerInfo::clearControlCache() {
    QMutexLocker locker(&m_mutex);
    for (QHash<ConfigKey, ControlObjectThread*>::const_iterator it = m_controlCache.begin();
            it != m_controlCache.end(); ++it) {
            qDebug() << "PlayerInfo::clearControlCache()";
            delete it.value();
    }
    m_controlCache.clear();
}
