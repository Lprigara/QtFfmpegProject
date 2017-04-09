#ifndef PRUEBA2_H
#define PRUEBA2_H

#include <QDebug>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

class videoScaler {
public:
    videoScaler();
    bool openInputFile(const char *filename);
    bool openOutputFile(const char *filename);
    void initCodec();
    void initVars();
    bool processScaled(const char* in_filename, const char* out_filename, int height, int width);
    AVFrame* scaleFrame(AVFrame *frame);
    bool encodeFrame(AVFrame *frame, int stream_index);
    bool muxEncodedPacket();

private:
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet, encodedPacket;
    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;
    AVCodecContext *videoDecoderCtx, *audioDecoderCtx, *videoEncoderCtx, *audioEncoderCtx;
    AVCodec *videoDecoder, *audioDecoder, *videoEncoder_, *audioEncoder;
    AVFrame* frameDst, *frame;
    struct SwsContext *imgConvertCtx;
    int heightDst, widthDst;
    int gotFrame;

};

#endif // PRUEBA2_H
