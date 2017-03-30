#ifndef AUDIOEXTRACTOR_H
#define AUDIOEXTRACTOR_H

#include <QDebug>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libavfilter/avfiltergraph.h>
  #include <libavfilter/avfilter.h>
  #include <libavfilter/buffersink.h>
  #include <libavfilter/buffersrc.h>
  #include "libavutil/opt.h"
}

#define PRIx64 "lx"

class audioExtractor
{
public:
    audioExtractor();
    void set_output_sample_fmt();
    AVStream* add_stream(AVCodec **codec, enum AVCodecID codec_id);
    int open_audio();
    bool write_audio_frame(AVFrame *frame);
    void flush_queue(AVCodecContext *codec);
    bool init_filter();
    int push_frame(AVFrame* frame);
    bool transcode();
    void init_audio_info();
    bool open_input_file(const char* inputFileName);
    bool open_output_file(const char* outputFileName);
    void init();
    bool convert(const char* inputFileName, const char* outputFileName);
    void avcodec_get_frame_defaults(AVFrame *frame);

private:
    AVCodecContext* inAudioCodecCtx, *outAudioCodecCtx;
    AVFormatContext* informatCtx, *outformatCtx;
    AVCodec* inAudioCodec, *outAudioCodec;
    int audioStreamIndex;
    AVFilterGraph *filterGraph;
    AVFilterContext *buffersinkCtx;
    AVFilterContext *buffersrcCtx;
    AVOutputFormat *outputFormat;
    AVStream *audioStream;
};

#endif // AUDIOEXTRACTOR_H
