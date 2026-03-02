#include "videowidget.h"
#include <QFileDialog>


VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(3);

    // תצוגת וידאו
    m_videoLabel = new QLabel("No Signal");
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(320, 200);
    m_videoLabel->setStyleSheet(
        "background: #1a1a1a; color: #666; border: 1px solid #333;"
        );

    // שורת URL
    auto* urlLayout = new QHBoxLayout();
    m_urlInput = new QLineEdit();
    m_urlInput->setPlaceholderText("rtsp://... or /path/to/file.mp4");

    m_webcamBtn = new QPushButton("📷 בחר מצלמה");
    m_webcamBtn->setFixedWidth(85);
    m_webcamBtn->setToolTip("בחר מצלמה");

    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setFixedWidth(80);
    connect(m_connectBtn, &QPushButton::clicked, this, &VideoWidget::onConnectClicked);
    connect(m_webcamBtn, &QPushButton::clicked, this, [this]() {
        QMenu* menu = new QMenu(this);

        QProcess process;
        process.start("C:/ffmpeg/bin/ffmpeg.exe",
                      {"-list_devices", "true", "-f", "dshow", "-i", "dummy"});
        process.waitForFinished(3000);
        QString output = process.readAllStandardError();

        QStringList lines = output.split("\n");
        bool foundVideo = false;
        for (const QString& line : lines) {
            if (line.contains("(video)")) foundVideo = true;
            if (foundVideo && line.contains("\"") && line.contains("(video)")) {
                int start = line.indexOf('"') + 1;
                int end   = line.indexOf('"', start);
                QString name = line.mid(start, end - start);
                if (!name.isEmpty()) {
                    menu->addAction("📷 " + name, [this, name]() {
                        m_urlInput->setText("video=" + name);
                    });
                }
            }
        }

        if (menu->isEmpty()) {
            menu->addAction("לא נמצאו מצלמות");
        }

        menu->exec(m_webcamBtn->mapToGlobal(
            QPoint(0, m_webcamBtn->height())));
    });

    m_fileBtn = new QPushButton("📁 בחר קובץ");
    m_fileBtn->setFixedWidth(80);
    m_fileBtn->setToolTip("בחר קובץ וידאו");
    connect(m_fileBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(
            this,
            "בחר קובץ וידאו",
            QDir::homePath(),
            "Video Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.ts *.m4v);;All Files (*.*)"
            );
        if (!file.isEmpty()) {
            // המר backslash ל-forward slash
            file = QDir::toNativeSeparators(file);
            m_urlInput->setText(file);
            startStream(file);
        }
    });

    urlLayout->addWidget(m_urlInput);
    urlLayout->addWidget(m_fileBtn);
    urlLayout->addWidget(m_webcamBtn);
    urlLayout->addWidget(m_connectBtn);

    // סרגל התקדמות
    m_seekSlider = new QSlider(Qt::Horizontal);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setValue(0);
    m_seekSlider->setEnabled(false);
    m_seekSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 6px; background: #333; border-radius: 3px; }"
        "QSlider::handle:horizontal { width: 14px; height: 14px; background: #0af; "
        "                             border-radius: 7px; margin: -4px 0; }"
        "QSlider::sub-page:horizontal { background: #0af; border-radius: 3px; }"
        );
    connect(m_seekSlider, &QSlider::sliderPressed,  [this]{ m_isSeeking = true; });
    connect(m_seekSlider, &QSlider::sliderReleased, [this]{
        m_isSeeking = false;
        onSeek(m_seekSlider->value());
    });

    // שורת פקדים
    auto* controlLayout = new QHBoxLayout();

    m_playPauseBtn = new QPushButton("⏸");
    m_playPauseBtn->setFixedWidth(36);
    m_playPauseBtn->setEnabled(false);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &VideoWidget::onPlayPauseClicked);

    m_timeLabel = new QLabel("0:00 / 0:00");
    m_timeLabel->setStyleSheet("color: #aaa; font-size: 11px;");

    QLabel* speedLabel = new QLabel("Speed:");
    speedLabel->setStyleSheet("color: #aaa; font-size: 11px;");

    m_speedCombo = new QComboBox();
    m_speedCombo->addItems({"0.25x", "0.5x", "1x", "1.5x", "2x"});
    m_speedCombo->setCurrentIndex(2);
    m_speedCombo->setEnabled(false);
    m_speedCombo->setFixedWidth(60);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VideoWidget::onSpeedChanged);

    // בקרת עוצמת קול
    m_volumeLabel = new QLabel("🔊");
    m_volumeLabel->setStyleSheet("color: #aaa; font-size: 13px;");

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 4px; background: #333; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 12px; height: 12px; background: #fff; "
        "                             border-radius: 6px; margin: -4px 0; }"
        "QSlider::sub-page:horizontal { background: #0af; border-radius: 2px; }"
        );
    connect(m_volumeSlider, &QSlider::valueChanged, this, &VideoWidget::onVolumeChanged);

    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addWidget(m_timeLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(speedLabel);
    controlLayout->addWidget(m_speedCombo);
    controlLayout->addWidget(m_volumeLabel);
    controlLayout->addWidget(m_volumeSlider);

    mainLayout->addWidget(m_videoLabel, 1);
    mainLayout->addLayout(urlLayout);
    mainLayout->addWidget(m_seekSlider);
    mainLayout->addLayout(controlLayout);
}

void VideoWidget::setupAudio(int sampleRate, int channels) {
    if (!m_audioPlayer) {
        m_audioPlayer = new AudioPlayer(this);
    }
    m_audioPlayer->setup(sampleRate, channels);
}

void VideoWidget::onConnectClicked() {
    if (m_decoder) stopStream();
    else {
        QString url = m_urlInput->text().trimmed();
        if (!url.isEmpty()) startStream(url);
    }
}

void VideoWidget::startStream(const QString& url) {
    stopStream();
    m_decoder = new VideoDecoder(url, this);
    connect(m_decoder, &VideoDecoder::frameReady,    this, &VideoWidget::onFrameReady);
    connect(m_decoder, &VideoDecoder::audioReady,    this, &VideoWidget::onAudioReady);
    connect(m_decoder, &VideoDecoder::errorOccurred, this, &VideoWidget::onError);
    m_decoder->start();
    m_isPlaying = true;
    m_connectBtn->setText("Disconnect");
    m_playPauseBtn->setText("⏸");
    m_playPauseBtn->setEnabled(true);
    m_seekSlider->setEnabled(true);
    m_speedCombo->setEnabled(true);
}

void VideoWidget::stopStream() {
    if (m_decoder) {
        m_decoder->stop();
        m_decoder->wait(3000);
        delete m_decoder;
        m_decoder = nullptr;
    }
    if (m_audioPlayer) {
        m_audioPlayer->cleanup();
        delete m_audioPlayer;
        m_audioPlayer = nullptr;
    }
    m_isPlaying = false;
    m_connectBtn->setText("Connect");
    m_playPauseBtn->setText("▶");
    m_playPauseBtn->setEnabled(false);
    m_seekSlider->setValue(0);
    m_seekSlider->setEnabled(false);
    m_speedCombo->setEnabled(false);
    m_timeLabel->setText("0:00 / 0:00");
    m_videoLabel->setText("No Signal");
}

void VideoWidget::onPlayPauseClicked() {
    if (!m_decoder) return;
    m_isPlaying = !m_isPlaying;
    m_decoder->setPaused(!m_isPlaying);
    m_playPauseBtn->setText(m_isPlaying ? "⏸" : "▶");
}

void VideoWidget::onSeek(int sliderValue) {
    if (!m_decoder) return;
    int total = m_seekSlider->maximum();
    int target = (int)((sliderValue / 1000.0) * total);
    m_decoder->seekTo(target);
}

void VideoWidget::onSpeedChanged(int index) {
    if (!m_decoder) return;
    double speeds[] = {0.25, 0.5, 1.0, 1.5, 2.0};
    m_decoder->setSpeed(speeds[index]);
}

void VideoWidget::onVolumeChanged(int value) {
    float vol = value / 100.0f;
    if (m_decoder) m_decoder->setVolume(vol);
    if (m_audioPlayer) m_audioPlayer->setVolume(vol);
    m_volumeLabel->setText(value == 0 ? "🔇" : value < 50 ? "🔉" : "🔊");
}

void VideoWidget::onAudioReady(QByteArray pcmData, int sampleRate, int channels) {
    if (!m_audioPlayer) setupAudio(sampleRate, channels);
    m_audioPlayer->addAudio(pcmData);
}
void VideoWidget::onFrameReady(QImage frame, int currentFrame, int totalFrames) {
    m_videoLabel->setPixmap(
        QPixmap::fromImage(frame).scaled(
            m_videoLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation)
        );

    if (totalFrames > 0 && !m_isSeeking) {
        m_seekSlider->setMaximum(totalFrames);
        m_seekSlider->setValue(currentFrame);

        double fps = 25.0;
        int cur = (int)(currentFrame / fps);
        int tot = (int)(totalFrames  / fps);
        m_timeLabel->setText(
            QString("%1:%2 / %3:%4")
                .arg(cur/60, 2, 10, QChar('0')).arg(cur%60, 2, 10, QChar('0'))
                .arg(tot/60, 2, 10, QChar('0')).arg(tot%60, 2, 10, QChar('0'))
            );
    }
}

void VideoWidget::onError(QString error) {
    m_videoLabel->setText("Error:\n" + error);
}
