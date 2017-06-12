#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

/**
 * @brief Clase que permite transcodificar las pistas del contenedor
 */
class videoTranscode
{
public:
    /**
     * @brief Constructor
     */
    videoTranscode();

    /**
     * @brief Inicialización de variables
     */
    void initVars();

    /**
     * @brief Inicialización de códecs
     */
    void initCodec();

    /**
     * @brief Abre el fichero de entrada y crea un contenedor de entrada
     * @param filename
     * @return
     */
    bool openInputFile(const char *filename);

    /**
     * @brief  Crea un contenedor de salida con la extensión del fichero destino
     * @param filename
     * @param audioCodec
     * @param videoCodec
     * @return
     */
    bool openOutputFile(const char *filename, AVCodecID audioCodec, AVCodecID videoCodec);

    /**
     * @brief Función principal encargada de transcodificar las pistas de audio y vídeo
     * @param inFilename
     * @param outFilename
     * @param audioCodec
     * @param videoCodec
     * @return
     */
    bool transcode(const char* inFilename, const char* outFilename, const char* audioCodec, const char* videoCodec);

    /**
     * @brief Codifica los frames modificados
     * @param frame
     * @param streamIndex
     * @param gotFrame
     * @return
     */
    bool encodeWriteFrame(AVFrame *frame, int streamIndex, int gotFrame);

    /**
     * @brief Obtener AudioCodecID
     * @param audioCodecStr
     * @return
     */
    AVCodecID getAudioCodecID(const char* audioCodecStr);

    /**
     * @brief Obtener VideoCodecID
     * @param videoCodecStr
     * @return
     */
    AVCodecID getVideoCodecID(const char *videoCodecStr);

    /**
     * @brief Escribe el frame modificado en el contenedor de salida
     * @param encodedPacket
     * @return
     */
    bool writeFrameInOutput(AVPacket encodedPacket);

private:
    AVOutputFormat *outputFormat;
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet;
    AVStream *inStream, *outStream;
    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;
    AVCodecContext *videoDecoderCtx, *audioDecoderCtx, *videoEncoderCtx, *audioEncoderCtx;
    AVCodec *videoDecoder, *audioDecoder, *videoEncoder_, *audioEncoder;
    AVCodecID videoCodec, audioCodec;
};

#endif // VIDEOENCODER_H
