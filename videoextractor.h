#ifndef VIDEOEXTRACTOR_H
#define VIDEOEXTRACTOR_H

#include <QDebug>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

class videoExtractor
{
public:
    videoExtractor();
    AVStream* add_stream(AVCodec **codec, enum AVCodecID codec_id);
    bool writeVideoFrame(AVFrame *frame);
    void flush_queue(AVCodecContext *codec);
    bool transcode();
    bool openInputFile(const char* inputFileName);
    bool open_output_file(const char* outputFileName);
    bool convert(const char* inputFileName, const char* outputFileName);
    void avcodec_get_frame_defaults(AVFrame *frame);

private:
    AVCodecContext* inVideoCodecCtx, *outVideoCodecCtx;
    AVFormatContext* informatCtx, *outformatCtx;
    AVCodec* inVideoCodec, *outVideoCodec;
    int videoStreamIndex;
    AVOutputFormat *outputFormat;
    AVStream *videoStream;
};

#endif // VIDEOEXTRACTOR_H
