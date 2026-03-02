#pragma once
#include <QThread>
#include <QImage>
#include <QString>
#include <QMutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

class VideoDecoder : public QThread {
    Q_OBJECT
public:
    explicit VideoDecoder(const QString& url, QObject* parent = nullptr);
    void stop();
    void setPaused(bool paused);
    void seekTo(int frameNumber);
    void setSpeed(double speed);
    void setVolume(float volume);

signals:
    void frameReady(QImage frame, int currentFrame, int totalFrames);
    void audioReady(QByteArray pcmData, int sampleRate, int channels);
    void errorOccurred(QString error);

protected:
    void run() override;

private:
    QString m_url;
    bool m_running = false;
    bool m_paused = false;
    double m_speed = 1.0;
    float m_volume = 1.0f;
    int m_seekTarget = -1;
    QMutex m_mutex;
};
