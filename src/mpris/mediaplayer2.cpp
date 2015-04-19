#include <QApplication>
#include <QWidget>
#include <QDebug>

#include "mediaplayer2.h"
#include "mixxx.h"
#include "soundsourceproxy.h"

MediaPlayer2::MediaPlayer2(MixxxMainWindow* pMixxx, QObject* parent)
    : QDBusAbstractAdaptor(parent),
      m_pMixxx(pMixxx) {
}

MediaPlayer2::~MediaPlayer2() {
}

bool MediaPlayer2::canQuit() const {
    return true;
}

bool MediaPlayer2::fullscreen() const {
    return m_pMixxx->isFullScreen();
}

void MediaPlayer2::setFullscreen(bool fullscreen) {
    m_pMixxx->slotViewFullScreen(fullscreen);
}

bool MediaPlayer2::canSetFullscreen() const {
    return true;
}

bool MediaPlayer2::canRaise() const {
    return true;
}

bool MediaPlayer2::hasTrackList() const {
    return false;
}

QString MediaPlayer2::identity() const {
    return "Mixxx";
}

QString MediaPlayer2::desktopEntry() const {
    return "mixxx";
}

QStringList MediaPlayer2::supportedUriSchemes() const {
    QStringList protocols;
    protocols.append("file");
    return protocols;
}

QStringList MediaPlayer2::supportedMimeTypes() const {
    return SoundSourceProxy::supportedMimeTypes();
}

void MediaPlayer2::Raise() {
    m_pMixxx->raise();
}

void MediaPlayer2::Quit() {
    QApplication::quit();
}


