#ifndef AUDIOEXTRACTOR_H
#define AUDIOEXTRACTOR_H

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}
/**
 * @brief Clase cuyas funciones permite extraer la pista de audio de un contenedor multimedia
 */
class audioExtractor
{
public:
    /**
     * @brief Constructor
     */
    audioExtractor();

    /**
     * @brief Establece el formato de salida de las muestras de audio
     */
    void set_output_sample_fmt();

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
    bool writeAudioFrame(AVFrame *frame);

    /**
     * @brief Vacía la cola de frames
     * @param codec
     */
    void flushQueue(AVCodecContext *codec);

    /**
     * @brief Decodifica y codifica los frames de la pista de audio, ignorando los correspondientes a la pista de vídeo
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
    void freeFrame(AVFrame *frame);

private:
    AVCodecContext* inAudioCodecCtx, *outAudioCodecCtx;
    AVFormatContext* informatCtx, *outformatCtx;
    AVCodec* inAudioCodec, *outAudioCodec;
    int audioStreamIndex;
    AVOutputFormat *outputFormat;
    AVStream *audioStream;
};

#endif // AUDIOEXTRACTOR_H
