#ifndef PRUEBA2_H
#define PRUEBA2_H

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

/**
 * @brief Clase que permite cambiar las dimensiones de los frames de la pista de vídeo
 */
class videoScaler {
public:
    /**
     * @brief Constructor
     */
    videoScaler();
    /**
     * @brief Inicializar códecs
     */
    void initCodec();
    /**
     * @brief Inicializar variables
     */
    void initVars();
    /**
     * @brief Abrir archivo multimedia de entrada
     * @param filename
     * @return
     */
    bool openInputFile(const char *filename);

    /**
     * @brief Crear contexto y fichero de salida
     * @param filename
     * @return
     */
    bool createOutputFile(const char *filename);
    /**
     * @brief Cambiar las dimensiones de los frames de la pista de vídeo
     * @param in_filename : ruta de archivo multimedia de entrada
     * @param out_filename : ruta de archivo multimedia de salida
     * @param height : alto deseada para los frames
     * @param width : ancho deseado para los frames
     * @return
     */
    bool scale(const char* in_filename, const char* out_filename, int height, int width);

    /**
     * @brief Cambia las dimensiones del frame
     * @param frame
     * @return
     */
    AVFrame* scaleFrame(AVFrame *frame);

    /**
     * @brief Codifica el frame modificado
     * @param frame
     * @param stream_index
     * @return
     */
    bool encodeFrame(AVFrame *frame, int stream_index);

    /**
     * @brief Multiplexa el frame codificado
     * @return
     */
    bool muxEncodedPacket();

private:
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVPacket packet, encodedPacket;
    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;
    AVCodecContext *videoDecoderCtx, *audioDecoderCtx, *videoEncoderCtx, *audioEncoderCtx;
    AVCodec *videoDecoder, *audioDecoder, *videoEncoder_, *audioEncoder;
    AVFrame* frameDst, *frame;
    struct SwsContext *imgConvertCtx;
    int heightDst, widthDst;
    int gotFrame;

};

#endif // PRUEBA2_H
