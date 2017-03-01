#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QFileDialog>
#include <QVideoFrame>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class videoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit videoDecoder(QObject *parent = 0);
    bool loadVideo(QString fileName);
    bool isOk();
    bool decodeAndDisplayFrames();
    bool initCodec();
    int getVideoLengthMs();
    void dumpFormat(AVFormatContext *ic, int index, const char *url, int is_output);
    QImage getLastFrame();
    bool isLastFrameOk();
    int getLastFrameTime();
    int getLastFrameNumber();

signals:
    void signalDisplayFrame(QImage lastFrame);

private:
    AVFormatContext *formatCtx;
    int videoStream, audioStream, numBytes;
    AVCodecContext *audioCodecCtx, *videoCodecCtx;
    AVCodec *audioCodec, *videoCodec;
    AVFrame *frame, *frameRGB;
    uint8_t *buffer;
    struct SwsContext *imgConvertCtx;
    bool ok;
    int desiredFrameTime;
    bool lastFrameOk;
    int lastFrameTime,lastFrameNumber;
    QImage lastFrame;
    AVPacket packet;

};

#endif // VIDEODECODE_H
