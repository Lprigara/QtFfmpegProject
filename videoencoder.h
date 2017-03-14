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
}

class videoEncoder
{
public:
    videoEncoder();

    void initVars();
    void initCodec();
    void remuxing(const char *in_filename, const char *out_filename);

private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
};

#endif // VIDEOENCODER_H
