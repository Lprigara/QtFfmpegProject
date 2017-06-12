#ifndef VIDEOEXTRACTOR_H
#define VIDEOEXTRACTOR_H

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

/**
 * @brief Clase cuyas funciones permite extraer la pista de vídeo de un contenedor multimedia
 */
class videoExtractor
{
public:
    /**
     * @brief Constructor
     */
    videoExtractor();

    /**
     * @brief Permite añadir un stream al contexto de salida
     * @param codec
     * @param codec_id
     * @return
     */
    AVStream* addStream(AVCodec **codec, enum AVCodecID codec_id);

    /**
     * @brief Escribe el contenido del frame en el contexto de salida
     * @param frame
     * @return
     */
    bool writeVideoFrame(AVFrame *frame);

    /**
     * @brief Vacía la cola de frames
     * @param codec
     */
    void flushQueue(AVCodecContext *codec);

    /**
     * @brief Decodifica y codifica los frames de la pista de vídeo, ignorando los correspondientes a la pista de audio
     * @return
     */
    bool transcode();

    /**
     * @brief Abre el fichero de entrada y crea un contenedor de entrada
     * @param inputFileName
     * @return
     */
    bool openInputFile(const char* inputFileName);

    /**
     * @brief Crea un contenedor de salida con la extensión del fichero destino
     * @param outputFileName
     * @return
     */
    bool openOutputFile(const char* outputFileName);

    /**
     * @brief Función principal, lleva a cabo la operación de extraer la pista de audio
     * @param inputFileName
     * @param outputFileName
     * @return
     */
    bool extract(const char* inputFileName, const char* outputFileName);

    /**
     * @brief Libera los frames para poder volverlos a asignar
     * @param frame
     */
    void freeFrames(AVFrame *frame);

private:
    AVCodecContext* inVideoCodecCtx, *outVideoCodecCtx;
    AVFormatContext* informatCtx, *outformatCtx;
    AVCodec* inVideoCodec, *outVideoCodec;
    int videoStreamIndex;
    AVOutputFormat *outputFormat;
    AVStream *videoStream;
};

#endif // VIDEOEXTRACTOR_H
