#ifndef VIDEOCUTTER_H
#define VIDEOCUTTER_H

extern "C" {
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
}

/**
 * @brief Clase cuya función permite extraer un segmento de vídeo de un archivo multimedia
 */
class videoCutter
{
public:
    /**
     * @brief Constructor
     */
    videoCutter();
    /**
     * @brief Función que permite extraer un segmento de vídeo
     * @param startTime : Tiempo de inicio deseado del segmento
     * @param endTime : Tiempo de fin deseado del segmento
     * @param inFilename : Ruta del archivo multimedia de entrada
     * @param outFilename : Ruta del archivo multimedia de salida
     * @return
     */
    bool cut(double startTime, double endTime, const char* inFilename, const char* outFilename);

private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
};

#endif // VIDEOCUTTER_H
