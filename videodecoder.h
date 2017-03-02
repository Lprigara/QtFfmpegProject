#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
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
    void initVariables();
    bool initCodec();

    bool loadVideo(QString fileName);
    void decodeFrame(int frameNumber);
    bool readNextFrame();
    void closeVideoAndClean();

    QImage getFrame();
    bool isLastFrameOk();
    int getLastFrameTime();
    int getLastFrameNumber();
    bool isVideoStream();
    bool isOk();
    bool isVideoFinished();

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
    bool videoFinished;
};

#endif // VIDEODECODE_H
