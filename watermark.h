#ifndef WATERMARK_H
#define WATERMARK_H

#include <stdio.h>
#include <QImage>

extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavfilter/avfiltergraph.h"
    #include "libavfilter/buffersink.h"
    #include "libavfilter/buffersrc.h"
    #include "libavutil/avutil.h"
    #include "libswscale/swscale.h"
    #include <libavresample/avresample.h>
    #include "SDL.h"
    #include <ao/ao.h>
    #include <libavutil/opt.h>

}

class watermark
{
public:
    watermark();
    int open_input_file(const char *filename);
    int init_filters(const char *filters_descr);
    int main(const char *inFilename, const char *waterMark, const char *outFilename);
    bool writeFrameInOutput(AVPacket encodedPacket);
    bool encodeWriteFrame(AVFrame *frame, int stream_index);
    bool openOutputFile(const char *filename);
    void checkDelays();
    void decodeAndPlayAudioSample();
    void setAudioFormat();
    bool convertImage(AVFrame frame);
    void decodeVideoFrame();
    bool readNextFrame();
    bool isVideoStream();
    bool isAudioStream();
    QImage getImage();
    void decodeAudioFrame();

private:

    AVFormatContext *pFormatCtx, *outFormatCtx;
    AVCodecContext *pCodecCtx;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
    int video_stream_index = -1;
    int audio_stream_index = -1;
    AVStream *inVideoStream, *outVideoStream;
    AVCodecContext *videoDecoderCtx, *videoEncoderCtx, *audioDecoderCtx;
    AVCodec *videoEncoder_, *videoDecoder, *audioDecoder;
    AVCodecID videoCodec;
    AVAudioResampleContext* resampleCtx;
    int frameFinished;
    uint8_t *output;
    int out_linesize;
    int out_samples;
    int64_t out_sample_fmt;
    ao_sample_format sformat;
    ao_info* info;
    int driver;
    ao_device *adevice;
    AVStream *video_st;
    AVPacket packet;
    AVFrame *audioFrame;
    QImage lastFrame;
    struct SwsContext *imgConvertCtx;
    AVFrame *pFrame;
    AVFrame *pFrame_out;


};

#endif // WATERMARK_H
