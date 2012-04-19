#include <QtDebug>

#include "trackinfoobject.h"
#include "analyserqueue.h"
#include "soundsourceproxy.h"

#ifdef __TONAL__
#include "tonal/tonalanalyser.h"
#endif

#include "analyserwaveform.h"
#include "analyserbpm.h"
#include "analyserrg.h"
#ifdef __VAMP__
#include "analyserbeats.h"
#include "analysergainvamp.h";
#include "vamp/vampanalyser.h"
#endif

#include <typeinfo>

AnalyserQueue::AnalyserQueue() : m_aq(),
    m_tioq(),
    m_qm(),
    m_qwait(),
    m_exit(false)
{

}

int AnalyserQueue::numQueuedTracks()
{
    m_qm.lock();
    int numQueuedTracks = m_tioq.count();
    m_qm.unlock();
    return numQueuedTracks;
}

void AnalyserQueue::addAnalyser(Analyser* an) {
    m_aq.push_back(an);
}

TrackPointer AnalyserQueue::dequeueNextBlocking() {
    m_qm.lock();

    if (m_tioq.isEmpty()) {
        m_qwait.wait(&m_qm);

        if (m_exit) {
            m_qm.unlock();
            return TrackPointer();
        }
    }

    // Implicit cast to TrackPointer from weak pointer
    TrackPointer pTrack = m_tioq.dequeue();

    m_qm.unlock();

    // pTrack might be NULL, up to the caller to check.
    return pTrack;
}

void AnalyserQueue::doAnalysis(TrackPointer tio, SoundSourceProxy *pSoundSource) {

    // TonalAnalyser requires a block size of 65536. Using a different value
    // breaks the tonal analyser. We need to use a smaller block size becuase on
    // Linux, the AnalyserQueue can starve the CPU of its resources, resulting
    // in xruns.. A block size of 8192 seems to do fine.
    const int ANALYSISBLOCKSIZE = 8192;

    int totalSamples = pSoundSource->length();
    //qDebug() << tio->getFilename() << " has " << totalSamples << " samples.";
    int processedSamples = 0;

    SAMPLE *data16 = new SAMPLE[ANALYSISBLOCKSIZE];
    CSAMPLE *samples = new CSAMPLE[ANALYSISBLOCKSIZE];

    int read = 0;
    bool dieflag = false;

    do {
        read = pSoundSource->read(ANALYSISBLOCKSIZE, data16);

        // Safety net in case something later barfs on 0 sample input
        if (read == 0) {
            break;
        }

        // If we get more samples than length, ask the analysers to process
        // up to the number we promised, then stop reading - AD
        if (read + processedSamples > totalSamples) {
            qDebug() << "While processing track of length " << totalSamples << " actually got "
                     << read + processedSamples << " samples, truncating analysis at expected length";
            read = totalSamples - processedSamples;
            dieflag = true;
        }

        for (int i = 0; i < read; i++) {
            samples[i] = ((float)data16[i])/32767.0f;
        }

        QListIterator<Analyser*> it(m_aq);

        while (it.hasNext()) {
            Analyser* an =  it.next();
            //qDebug() << typeid(*an).name() << ".process()";
            an->process(samples, read);
            //qDebug() << "Done " << typeid(*an).name() << ".process()";
        }

        // emit progress updates to whoever cares
        processedSamples += read;
        int progress = ((float)processedSamples)/totalSamples * 100; //fp div here prevents insano signed overflow
        emit(trackProgress(tio, progress));

        // Since this is a background analysis queue, we should co-operatively
        // yield every now and then to try and reduce CPU contention. The
        // analyser queue is CPU intensive so we want to get out of the way of
        // the audio callback thread.
        //QThread::yieldCurrentThread();
        //QThread::usleep(10);

    } while(read == ANALYSISBLOCKSIZE && !dieflag);

    delete[] data16;
    delete[] samples;
}

void AnalyserQueue::stop() {
    m_exit = true;
}

void AnalyserQueue::run() {

    unsigned static id = 0; //the id of this thread, for debugging purposes //XXX copypasta (should factor this out somehow), -kousu 2/2009
    QThread::currentThread()->setObjectName(QString("AnalyserQueue %1").arg(++id));

    // If there are no analysers, don't waste time running.
    if (m_aq.size() == 0)
        return;

    while (!m_exit) {
        TrackPointer next = dequeueNextBlocking();

        if (m_exit) //When exit is set, it makes the above unblock first.
            return;

        // If the track is NULL, get the next one. Could happen if the track was
        // queued but then deleted.
        if (!next)
            continue;

        // Get the audio
        SoundSourceProxy * pSoundSource = new SoundSourceProxy(next);
        pSoundSource->open(); //Open the file for reading
        int iNumSamples = pSoundSource->length();
        int iSampleRate = pSoundSource->getSampleRate();

        if (iNumSamples == 0 || iSampleRate == 0) {
            qDebug() << "Skipping invalid file:" << next->getLocation();
            continue;
        }

        QListIterator<Analyser*> it(m_aq);

        while (it.hasNext()) {
            it.next()->initialise(next, iSampleRate, iNumSamples);
        }

        doAnalysis(next, pSoundSource);

        QListIterator<Analyser*> itf(m_aq);

        while (itf.hasNext()) {
            itf.next()->finalise(next);
        }

        delete pSoundSource;
        emit(trackFinished(next));

        m_qm.lock();
        bool empty = m_tioq.isEmpty();
        m_qm.unlock();
        if (empty) {
            emit(queueEmpty());
        }
    }
}

void AnalyserQueue::queueAnalyseTrack(TrackPointer tio) {
    m_qm.lock();
    if( !m_tioq.contains(tio)){
        m_tioq.enqueue(tio);
        m_qwait.wakeAll();
    }
    m_qm.unlock();
}

AnalyserQueue* AnalyserQueue::createAnalyserQueue(QList<Analyser*> analysers) {
    AnalyserQueue* ret = new AnalyserQueue();

    QListIterator<Analyser*> it(analysers);
    while(it.hasNext()) {
        ret->addAnalyser(it.next());
    }

    ret->start(QThread::IdlePriority);
    return ret;
}

AnalyserQueue* AnalyserQueue::createDefaultAnalyserQueue(ConfigObject<ConfigValue> *_config) {
    AnalyserQueue* ret = new AnalyserQueue();

#ifdef __TONAL__
    ret->addAnalyser(new TonalAnalyser());
#endif

    ret->addAnalyser(new AnalyserWaveform(_config));

#ifdef __VAMP__
    VampAnalyser::initializePluginPaths();
    ret->addAnalyser(new AnalyserGainVamp(_config));
    ret->addAnalyser(new AnalyserBeats(_config));
    //ret->addAnalyser(new AnalyserVampKeyTest(_config));
#else
    ret->addAnalyser(new AnalyserBPM(_config));
    ret->addAnalyser(new AnalyserGain(_config));
#endif
    ret->start(QThread::IdlePriority);
    return ret;
}

AnalyserQueue* AnalyserQueue::createPrepareViewAnalyserQueue(ConfigObject<ConfigValue> *_config) {
    AnalyserQueue* ret = new AnalyserQueue();
#ifdef __VAMP__
    VampAnalyser::initializePluginPaths();
    ret->addAnalyser(new AnalyserGainVamp(_config));
    ret->addAnalyser(new AnalyserBeats(_config));
    //ret->addAnalyser(new AnalyserVampKeyTest(_config));
#else
    ret->addAnalyser(new AnalyserBPM(_config));
    ret->addAnalyser(new AnalyserGain(_config));
#endif
    ret->start(QThread::IdlePriority);
    return ret;
}

AnalyserQueue::~AnalyserQueue() {
    QListIterator<Analyser*> it(m_aq);

    stop();

    m_qm.lock();
    m_qwait.wakeAll();
    m_qm.unlock();

    wait(); //Wait until thread has actually stopped before proceeding.

    while (it.hasNext()) {
        Analyser* an = it.next();
        //qDebug() << "AnalyserQueue: deleting " << typeid(an).name();
        delete an;
    }
}
