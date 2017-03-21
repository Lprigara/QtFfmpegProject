#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QString>
#include <QDebug>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include<libavutil/opt.h>
    #include<libavutil/avstring.h>
    #include <libavutil/timestamp.h>
    #include <libavutil/avassert.h>
    #include <libavutil/mathematics.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/avfiltergraph.h>
    #include <libavfilter/buffersrc.h>
}


class videoEncoder
{
public:
    videoEncoder();

    void initVars();
    void initCodec();
    bool remuxing(const char *inFilename, const char *outFilename);
    bool openInputFile(const char *filename);
    bool openOutputFile(const char *filename, AVCodecID audioCodec, AVCodecID videoCodec);
    bool transcode(const char* inFilename, const char* outFilename, QString audioCodec, QString videoCodec);
    bool encodeWriteFrame(AVFrame *frame, int streamIndex, int gotFrame);
    AVCodecID getAudioCodecID(QString audioCodecStr);
    AVCodecID getVideoCodecID(QString videoCodecStr);
    bool writeFrameInOutput(AVPacket encodedPacket);



private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;
    AVCodecContext *videoDecoderCtx, *audioDecoderCtx, *videoEncoderCtx, *audioEncoderCtx;
    AVCodec *videoDecoder, *audioDecoder, *videoEncoder_, *audioEncoder;
    AVCodecID videoCodec, audioCodec;
};

#endif // VIDEOENCODER_H
