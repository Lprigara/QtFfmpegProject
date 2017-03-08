#ifndef VIDEODETAILS_H
#define VIDEODETAILS_H

#include <QFile>
#include <QTextStream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavresample/avresample.h>
    #include <ao/ao.h>
    #include <libavutil/opt.h>
    #include <libavutil/avstring.h>
}

class videoInfo
{
public:
    videoInfo(const char* ruta, const char* nombre);
    void setRuta(const char* ruta);
    void setNombre(const char* nombre);
    const char* getRuta();
    const char* getNombre();

private:
    const char* ruta;
    const char* nombre;
};

#endif // VIDEODETAILS_H
