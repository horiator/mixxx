#ifndef ANALYSERQUEUE_H
#define ANALYSERQUEUE_H

#include <QList>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QSemaphore>

#include "configobject.h"
#include "analyser.h"
#include "trackinfoobject.h"

class SoundSourceProxy;

class AnalyserQueue : public QThread {
    Q_OBJECT

  public:
    AnalyserQueue();
    virtual ~AnalyserQueue();
    void stop();
    void queueAnalyseTrack(TrackPointer tio);

    static AnalyserQueue* createDefaultAnalyserQueue(ConfigObject<ConfigValue> *_config);
    static AnalyserQueue* createPrepareViewAnalyserQueue(ConfigObject<ConfigValue> *_config);
    static AnalyserQueue* createAnalyserQueue(QList<Analyser*> analysers);

  public slots:
    void slotAnalyseTrack(TrackPointer tio);
    void slotUpdateProgress();

  signals:
    void trackProgress(TrackPointer pTrack, int progress);
    void trackFinished(int size);
    void queueEmpty();
    void updateProgress();

  protected:
    void run();

  private:

    struct progress_info {
        TrackPointer current_track;
        int track_progress; // in 0.1 %
        int queue_size;
        QSemaphore sema;
    };

    void addAnalyser(Analyser* an);

    QList<Analyser*> m_aq;

    bool isLoadedTrackWaiting();
    TrackPointer dequeueNextBlocking();
    bool doAnalysis(TrackPointer tio, SoundSourceProxy *pSoundSource);
    void emitUpdateProgress(int progress);

    bool m_exit;
    QAtomicInt m_aiCheckPriorities;

    // The processing queue and associated mutex
    QQueue<TrackPointer> m_analyserTpQ;
    QQueue<TrackPointer> m_loadedTpQ;
    QMutex m_qm;
    QWaitCondition m_qwait;
    struct progress_info m_progressInfo;
};

#endif
