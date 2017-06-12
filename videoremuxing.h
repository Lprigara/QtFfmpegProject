#ifndef VIDEOREMUXING_H
#define VIDEOREMUXING_H


extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

/**
 * @brief Clase que permite realizar un remuxing o cambio de formato de un archivo multimedia
 */
class videoRemuxing
{
public:
    /**
     * @brief Constructor
     */
    videoRemuxing();

    /**
     * @brief Inicialización de códecs
     */
    void initCodec();

    /**
     * @brief Inicialización de variables
     */
    void initVars();

    /**
     * @brief Función principal encargada de realizar el cambio de formato de un archivo multimedia
     * @param inFilename
     * @param outFilename
     * @return
     */
    bool remuxing(const char* inFilename, const char* outFilename);

private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
};

#endif // VIDEOREMUXING_H
