#pragma once
#include <QObject>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMutex>
#include <QByteArray>
#include <QIODevice>
#include <QDebug>

class AudioBuffer : public QIODevice {
    Q_OBJECT
public:
    explicit AudioBuffer(QObject* parent = nullptr) : QIODevice(parent) {
        open(QIODevice::ReadWrite);
    }

    void addData(const QByteArray& data) {
        QMutexLocker lock(&m_mutex);
        m_buffer.append(data);
        if (m_buffer.size() > 192000)
            m_buffer = m_buffer.mid(m_buffer.size() - 192000);
    }

    qint64 bytesAvailable() const override {
        QMutexLocker lock(&m_mutex);
        return m_buffer.size() + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override {
        QMutexLocker lock(&m_mutex);
        if (m_buffer.isEmpty()) {
            memset(data, 0, maxSize);
            return maxSize;
        }
        qint64 toRead = qMin(maxSize, (qint64)m_buffer.size());
        memcpy(data, m_buffer.constData(), toRead);
        m_buffer.remove(0, toRead);
        return toRead;
    }

    qint64 writeData(const char*, qint64) override { return 0; }

private:
    QByteArray m_buffer;
    mutable QMutex m_mutex;
};

class AudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject* parent = nullptr) : QObject(parent) {}

    void setup(int sampleRate, int channels) {
        cleanup();
        QAudioFormat fmt;
        fmt.setSampleRate(sampleRate);
        fmt.setChannelCount(channels);
        fmt.setSampleFormat(QAudioFormat::Int16);

        m_audioBuffer = new AudioBuffer(this);
        m_sink = new QAudioSink(fmt, this);
        m_sink->setBufferSize(176400);
    }

    void addAudio(const QByteArray& data) {
        if (!m_audioBuffer) return;
        m_audioBuffer->addData(data);

        if (m_sink->state() == QAudio::StoppedState ||
            m_sink->state() == QAudio::SuspendedState) {
            if (m_audioBuffer->bytesAvailable() >= 88200) {
                m_sink->start(m_audioBuffer);
            }
        }
    }
    void setVolume(float vol) {
        if (m_sink) m_sink->setVolume(vol);
    }

    void cleanup() {
        if (m_sink) {
            m_sink->stop();
            delete m_sink;
            m_sink = nullptr;
        }
        m_audioBuffer = nullptr;
    }
private:
    QAudioSink*  m_sink = nullptr;
    AudioBuffer* m_audioBuffer = nullptr;
};
