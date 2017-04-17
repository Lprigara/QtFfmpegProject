#ifndef AUDIOEXTRACTOR_H
#define AUDIOEXTRACTOR_H

#include <QDebug>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

class audioExtractor
{
public:
    audioExtractor();
    void set_output_sample_fmt();
    AVStream* add_stream(AVCodec **codec, enum AVCodecID codec_id);
    bool write_audio_frame(AVFrame *frame);
    void flush_queue(AVCodecContext *codec);
    bool transcode();
    bool open_input_file(const char* inputFileName);
    bool open_output_file(const char* outputFileName);
    bool convert(const char* inputFileName, const char* outputFileName);
    void avcodec_get_frame_defaults(AVFrame *frame);

private:
    AVCodecContext* inAudioCodecCtx, *outAudioCodecCtx;
    AVFormatContext* informatCtx, *outformatCtx;
    AVCodec* inAudioCodec, *outAudioCodec;
    int audioStreamIndex;
    AVOutputFormat *outputFormat;
    AVStream *audioStream;
};

#endif // AUDIOEXTRACTOR_H
