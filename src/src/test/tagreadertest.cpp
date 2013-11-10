#include <gtest/gtest.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QAbstractSocket>
#include <qmetatype.h>

#include "trackinfoobject.h"
#include "soundsourcemp3.h"
#include "core/tagreaderclient.h"

#define BIGBUF_SIZE (1024 * 1024)  //Megabyte
#define CANARY_SIZE (1024*4)
#define MAGIC_FLOAT 1234.567890f
#define CANARY_FLOAT 0.0f

namespace {

class TagreaderTest: public testing::Test {
  protected:
    virtual void SetUp() {
        qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
        //QAbstractSocket::SocketState' is registered using qRegisterMetaType()

        QFile file("src/test/tagreadertest.mp3");
        file.copy("src/test/temptagreadertest.mp3");
        m_file = new QFile("src/test/temptagreadertest.mp3");

        m_tag_reader_client = TagReaderClient::Instance();
        if (!m_tag_reader_client) {
            qDebug() << "Starting TagreaderClient";
            m_tag_reader_client = new TagReaderClient();
            m_thread = new QThread();
            m_tag_reader_client->setParent(NULL);
            m_tag_reader_client->moveToThread(m_thread);
            m_thread->start();
            m_tag_reader_client->Start();
            while (!TagReaderClient::Instance()){
                sleep(1);
                qDebug() << ".";
            }
        }
        else
        {
            qDebug() << "TagReader Already started";
        }
    }

    virtual void TearDown() {
        m_file->remove();
        delete m_file;
        delete m_tag_reader_client;
        delete m_thread;
    }

    QFile* m_file;
    TagReaderClient* m_tag_reader_client;
    QThread* m_thread;
};

TEST_F(TagreaderTest, corruptionByExternalWrite) {
    EXPECT_TRUE(m_file->exists());

    // Load reference Track
    TrackPointer pTrack(new TrackInfoObject(m_file->fileName()), &QObject::deleteLater);
    qDebug() << pTrack->getArtist();
    EXPECT_TRUE(pTrack->getArtist().isEmpty());

    EXPECT_TRUE(m_file->open(QIODevice::ReadOnly));

    // Get a pointer to the file using memory mapped IO
    unsigned inputbuf_len = m_file->size();
    qDebug() << inputbuf_len;

    // Open Track mmaped
    uchar* inputbuf = m_file->map(0, inputbuf_len);

    // Loads First 4k Page
    EXPECT_TRUE(inputbuf[0x00000] == 0xff);
    qDebug() << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    // Save Artist via external process Tagreader
    // Byte at 0x00000 from 0xFF -> 0x49 (Page currently loaded)
    // byte at 0x10374 from 0xAA -> 0xFF (Page not loaded yet)
    pTrack->setArtist(QString("tagreader"));
    EXPECT_TRUE(m_tag_reader_client->SaveFileBlocking(pTrack->getLocation(),*pTrack));
    qDebug() << pTrack->getArtist();

    // Check First 4k Page again
    EXPECT_TRUE(inputbuf[0x00000] == 0x49);
    qDebug() << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    // Read Artist from real File, to check if it has changed
    TrackPointer pTrack2(new TrackInfoObject(m_file->fileName()), &QObject::deleteLater);
    EXPECT_TRUE(pTrack2->getArtist() == "tagreader");
    qDebug() << pTrack2->getArtist();

    // File Size has changed
    EXPECT_TRUE(inputbuf_len < m_file->size());

    // Check First 4k Page again
    qDebug() << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    // Check Byte in a new page
    qDebug() << QString("0x%1").arg(inputbuf[0x10374], 0, 16);
}

TEST_F(TagreaderTest, CopyWrite) {
    EXPECT_TRUE(m_file->exists());

    EXPECT_TRUE(m_file->open(QIODevice::ReadOnly));

    // Get a pointer to the file using memory mapped IO
    unsigned inputbuf_len = m_file->size();
    qDebug() << inputbuf_len;

    // Open Track mmaped
    uchar* inputbuf = m_file->map(0, inputbuf_len);

    // Loads First 4k Page
    EXPECT_TRUE(inputbuf[0x00000] == 0xff);
    qDebug() << "read from first memory page after map" << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    m_file->copy("src/test/temptagreadertestcopy.mp3");
    QFile filecopy("src/test/temptagreadertestcopy.mp3");

    // Load reference Track
    TrackPointer pTrack(new TrackInfoObject(filecopy.fileName()), &QObject::deleteLater);
    qDebug() << pTrack->getArtist();
    EXPECT_TRUE(pTrack->getArtist().isEmpty());

    // Save Artist via external process Tagreader
    // Byte at 0x00000 from 0xFF -> 0x49 (Page currently loaded)
    // byte at 0x10374 from 0xAA -> 0xFF (Page not loaded yet)
    pTrack->setArtist(QString("tagreader"));
    EXPECT_TRUE(m_tag_reader_client->SaveFileBlocking(pTrack->getLocation(),*pTrack));
    qDebug() << pTrack->getArtist();

    // Move file
    QFile::remove("src/test/temptagreadertest.mp3");
    EXPECT_TRUE(filecopy.rename("src/test/temptagreadertest.mp3"));

    // Check First 4k Page againm it should be still pointing to the old file maped in Memory
    EXPECT_TRUE(inputbuf[0x00000] == 0xff);
    qDebug() << "read from first memory page after move" << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    // Read Artist from real File, to check if it has changed
    TrackPointer pTrack2(new TrackInfoObject(m_file->fileName()), &QObject::deleteLater);
    EXPECT_TRUE(pTrack2->getArtist() == "tagreader");
    qDebug() << "read artist from written file" << pTrack2->getArtist();

    // File Size has changed
    EXPECT_TRUE(inputbuf_len < m_file->size());

    // Check First 4k Page again
    qDebug() << QString("0x%1").arg(inputbuf[0x00000], 0, 16);

    // Check Byte in a new page
    qDebug() << QString("0x%1").arg(inputbuf[0x10374], 0, 16);
}

}
