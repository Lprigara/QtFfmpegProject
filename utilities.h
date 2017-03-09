#ifndef UTILITIES_H
#define UTILITIES_H

#include <QTextStream>
#include <QFile>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavresample/avresample.h>
    #include <libavutil/opt.h>
    #include <libavutil/avstring.h>
    #include "libavutil/replaygain.h"
    #include "libavutil/spherical.h"
    #include "libavutil/stereo3d.h"
    #include "libavutil/intreadwrite.h"
}

class utilities{
    public:
        utilities();

        static void dumpFormat(QFile *file, AVFormatContext* formatCtx, int index, int is_output);
        static void dumpStreamFormat(QTextStream &outFile, AVFormatContext *formatCtx, int i, int index, int is_output);
        static void dumpMetadata(QTextStream &outFile, AVDictionary *metadata);
        static void dumpSidedata(QTextStream &outFile, AVStream *stream);
        static void dumpStereo3d(QTextStream &outFile, AVPacketSideData *sideData);
        static void dumpReplaygain(QTextStream &outFile, AVPacketSideData *sideData);
        static void dumpParamChange(QTextStream &outFile, AVPacketSideData *sideData);
        static void dumpAudioServiceType(QTextStream &outFile, AVPacketSideData *sideData);
        static void dumpCPB(QTextStream &outFile, AVPacketSideData *sideData);



};

#endif // UTILITIES_H
