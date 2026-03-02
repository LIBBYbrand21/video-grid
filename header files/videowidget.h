#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QProcess>
#include <QMenu>
#include "audioplayer.h"
#include "videodecoder.h"

class VideoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);
    void startStream(const QString& url);
    void stopStream();

private slots:
    void onFrameReady(QImage frame, int currentFrame, int totalFrames);
    void onAudioReady(QByteArray pcmData, int sampleRate, int channels);
    void onError(QString error);
    void onConnectClicked();
    void onPlayPauseClicked();
    void onSeek(int value);
    void onSpeedChanged(int index);
    void onVolumeChanged(int value);

private:
    QLabel*       m_videoLabel;
    QLineEdit*    m_urlInput;
    QPushButton*  m_connectBtn;
    QPushButton*  m_webcamBtn;
    QPushButton*  m_fileBtn;
    QPushButton*  m_playPauseBtn;
    QSlider*      m_seekSlider;
    QSlider*      m_volumeSlider;
    QLabel*       m_volumeLabel;
    QComboBox*    m_speedCombo;
    QLabel*       m_timeLabel;
    VideoDecoder* m_decoder  = nullptr;
    AudioPlayer*  m_audioPlayer = nullptr;
    bool m_isPlaying = false;
    bool m_isSeeking = false;

    void setupAudio(int sampleRate, int channels);
};
