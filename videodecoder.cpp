#include "videodecoder.h"
#include <QThread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavdevice/avdevice.h>
}

VideoDecoder::VideoDecoder(const QString& url, QObject* parent)
    : QThread(parent), m_url(url) {}

void VideoDecoder::stop()    { m_running = false; }
void VideoDecoder::setPaused(bool p) { QMutexLocker l(&m_mutex); m_paused = p; }
void VideoDecoder::seekTo(int f)     { QMutexLocker l(&m_mutex); m_seekTarget = f; }
void VideoDecoder::setSpeed(double s){ QMutexLocker l(&m_mutex); m_speed = s; }
void VideoDecoder::setVolume(float v){ QMutexLocker l(&m_mutex); m_volume = v; }

void VideoDecoder::run() {
    m_running = true;
    avdevice_register_all();

    AVFormatContext* fmtCtx = nullptr;
    AVDictionary* opts = nullptr;

    bool isWebcam = m_url.startsWith("video=");

    if (isWebcam) {
        const AVInputFormat* fmt = av_find_input_format("dshow");
        if (!fmt) {
            emit errorOccurred("dshow not available");
            return;
        }
        av_dict_set(&opts, "video_size", "1280x720", 0);
        av_dict_set(&opts, "framerate", "30", 0);

        QByteArray urlBytes = m_url.toLocal8Bit();
        int ret = avformat_open_input(&fmtCtx, urlBytes.constData(), fmt, &opts);
        if (ret < 0) {
            // נסה ברזולוציה נמוכה יותר
            av_dict_free(&opts);
            opts = nullptr;
            av_dict_set(&opts, "video_size", "640x480", 0);
            av_dict_set(&opts, "framerate", "30", 0);
            ret = avformat_open_input(&fmtCtx, urlBytes.constData(), fmt, &opts);
        }
        if (ret < 0) {
            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));
            emit errorOccurred("Webcam error: " + QString(errbuf));
            return;
        }
    } else {
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "stimeout", "5000000", 0);
        int ret = avformat_open_input(&fmtCtx, m_url.toStdString().c_str(), nullptr, &opts);
        if (ret < 0) {
            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));
            emit errorOccurred("Cannot open: " + QString(errbuf));
            return;
        }
    }

    avformat_find_stream_info(fmtCtx, nullptr);

    int videoStream = -1, audioStream = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        auto type = fmtCtx->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO && videoStream < 0) videoStream = i;
        if (type == AVMEDIA_TYPE_AUDIO && audioStream < 0) audioStream = i;
    }

    if (videoStream < 0) {
        emit errorOccurred("No video stream found");
        avformat_close_input(&fmtCtx);
        return;
    }

    AVStream* vStream = fmtCtx->streams[videoStream];
    const AVCodec* vCodec = avcodec_find_decoder(vStream->codecpar->codec_id);
    AVCodecContext* vCtx = avcodec_alloc_context3(vCodec);
    avcodec_parameters_to_context(vCtx, vStream->codecpar);
    avcodec_open2(vCtx, vCodec, nullptr);

    AVCodecContext* aCtx = nullptr;
    SwrContext* swrCtx = nullptr;
    if (audioStream >= 0) {
        AVStream* aStream = fmtCtx->streams[audioStream];
        const AVCodec* aCodec = avcodec_find_decoder(aStream->codecpar->codec_id);
        aCtx = avcodec_alloc_context3(aCodec);
        avcodec_parameters_to_context(aCtx, aStream->codecpar);
        avcodec_open2(aCtx, aCodec, nullptr);

        swrCtx = swr_alloc();
        AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inLayout = aCtx->ch_layout;
        swr_alloc_set_opts2(&swrCtx,
                            &stereoLayout, AV_SAMPLE_FMT_S16, 44100,
                            &inLayout, aCtx->sample_fmt, aCtx->sample_rate,
                            0, nullptr);
        swr_init(swrCtx);
    }

    int totalFrames = (int)vStream->nb_frames;
    if (totalFrames <= 0 && vStream->duration > 0) {
        double fps = av_q2d(vStream->avg_frame_rate);
        double dur = vStream->duration * av_q2d(vStream->time_base);
        totalFrames = (int)(fps * dur);
    }

    AVFrame* frame   = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    SwsContext* swsCtx = nullptr;
    int currentFrame = 0;

    while (m_running) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_seekTarget >= 0) {
                double fps = av_q2d(vStream->avg_frame_rate);
                if (fps > 0) {
                    double seconds = m_seekTarget / fps;
                    int64_t ts = (int64_t)(seconds / av_q2d(vStream->time_base));
                    av_seek_frame(fmtCtx, videoStream, ts, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(vCtx);
                    if (aCtx) avcodec_flush_buffers(aCtx);
                    currentFrame = m_seekTarget;
                }
                m_seekTarget = -1;
            }
        }

        {
            QMutexLocker locker(&m_mutex);
            if (m_paused) { locker.unlock(); QThread::msleep(50); continue; }
        }

        if (av_read_frame(fmtCtx, packet) < 0) break;

        if (packet->stream_index == videoStream) {
            if (avcodec_send_packet(vCtx, packet) == 0) {
                while (avcodec_receive_frame(vCtx, frame) == 0) {
                    swsCtx = sws_getCachedContext(swsCtx,
                                                  frame->width, frame->height, (AVPixelFormat)frame->format,
                                                  frame->width, frame->height, AV_PIX_FMT_RGB24,
                                                  SWS_BILINEAR, nullptr, nullptr, nullptr);

                    QImage image(frame->width, frame->height, QImage::Format_RGB888);
                    uint8_t* dest[1] = { image.bits() };
                    int destLinesize[1] = { (int)image.bytesPerLine() };
                    sws_scale(swsCtx, frame->data, frame->linesize,
                              0, frame->height, dest, destLinesize);

                    currentFrame++;
                    emit frameReady(image, currentFrame, totalFrames);

                    double fps = av_q2d(vStream->avg_frame_rate);
                    double speed;
                    { QMutexLocker l(&m_mutex); speed = m_speed; }
                    if (fps > 0 && speed > 0)
                        QThread::msleep((int)(1000.0 / (fps * speed)));
                }
            }
        }

        if (aCtx && packet->stream_index == audioStream) {
            if (avcodec_send_packet(aCtx, packet) == 0) {
                while (avcodec_receive_frame(aCtx, frame) == 0) {
                    int outSamples = (int)av_rescale_rnd(
                        frame->nb_samples, 44100, aCtx->sample_rate, AV_ROUND_UP);

                    QByteArray pcm(outSamples * 2 * 2, 0);
                    uint8_t* outData = (uint8_t*)pcm.data();

                    int converted = swr_convert(swrCtx,
                                                &outData, outSamples,
                                                (const uint8_t**)frame->data, frame->nb_samples);

                    if (converted > 0) {
                        pcm.resize(converted * 2 * 2);

                        float vol;
                        { QMutexLocker l(&m_mutex); vol = m_volume; }
                        int16_t* samples = (int16_t*)pcm.data();
                        for (int i = 0; i < pcm.size() / 2; i++)
                            samples[i] = (int16_t)(samples[i] * vol);

                        emit audioReady(pcm, 44100, 2);
                    }
                }
            }
        }

        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&vCtx);
    if (aCtx) avcodec_free_context(&aCtx);
    if (swrCtx) swr_free(&swrCtx);
    if (swsCtx) sws_freeContext(swsCtx);
    avformat_close_input(&fmtCtx);
}
