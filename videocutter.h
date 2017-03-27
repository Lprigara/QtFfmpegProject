#ifndef VIDEOCUTTER_H
#define VIDEOCUTTER_H

#include <QDebug>

extern "C" {
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
}

class videoCutter
{
public:
    videoCutter();
    bool cutVideo(double startTime, double endTime, const char* inFilename, const char* outFilename);

private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
};

#endif // VIDEOCUTTER_H
