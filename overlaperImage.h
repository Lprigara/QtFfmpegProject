#ifndef WATERMARK_H
#define WATERMARK_H

#include <stdio.h>
#include <QImage>

extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavfilter/avfiltergraph.h"
    #include "libavfilter/buffersink.h"
    #include "libavfilter/buffersrc.h"
    #include "libavutil/avutil.h"
    #include "libswscale/swscale.h"
    #include <libavresample/avresample.h>
    #include "SDL.h"
    #include <ao/ao.h>
    #include <libavutil/opt.h>

}

/**
 * @brief Clase que permite superponer una imagen a un canal de vídeo
 */
class overlaperImage
{
public:

    /**
     * @brief Constructor
     */
    overlaperImage();

    /**
     * @brief Abre el fichero de entrada y crea un contenedor para almacenar su información
     * @param filename
     * @return
     */
    int openInputFile(const char *filename);

    /**
     * @brief Inicializa los filtros almacenados en el parámetro filters_descr
     * @param filters_descr
     * @return
     */
    int initFilters(const char *filters_descr);

    /**
     * @brief Función principal, encargada de superponer una imagen a un canal de vídeo
     * @param inFilename
     * @param waterMark
     * @param outFilename
     * @return
     */
    int overlap(const char *inFilename, const char *waterMark, const char *outFilename);

    /**
     * @brief Escribe los frames modificados en el contenedor de salida
     * @param encodedPacket
     * @return
     */
    bool writeFrameInOutput(AVPacket encodedPacket);

    /**
     * @brief Codifica los frames modificados por el filtro
     * @param frame
     * @param stream_index
     * @return
     */
    bool encodeWriteFrame(AVFrame *frame, int stream_index);

    /**
     * @brief Crea un contenedor de salida correspondiente a la extensión del archivo destino
     * @param filename
     * @return
     */
    bool createOutputFile(const char *filename);

    /**
     * @brief Comprueba los retrasos
     */
    void checkDelays();

    /**
     * @brief Reproduce las muestras de audio
     */
    void playAudioSample();

    /**
     * @brief Establece el formato del canal audio
     */
    void setAudioFormat();

    /**
     * @brief Convierte el frame en imagen para poder reproducirlo
     * @param frame
     * @return
     */
    bool convertImage(AVFrame frame);

    /**
     * @brief Decodifica el frame de vídeo
     */
    void decodeVideoFrame();

    /**
     * @brief Leer el frame siguiente
     * @return
     */
    bool readNextFrame();

    /**
     * @brief isVideoStream
     * @return
     */
    bool isVideoStream();

    /**
     * @brief isAudioStream
     * @return
     */
    bool isAudioStream();

    /**
     * @brief getImage
     * @return
     */
    QImage getImage();

    /**
     * @brief Decodifica la muestra de audio
     */
    void decodeAudioFrame();

private:

    AVFormatContext *pFormatCtx, *outFormatCtx;
    AVCodecContext *pCodecCtx;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
    int video_stream_index = -1;
    int audio_stream_index = -1;
    AVStream *inVideoStream, *outVideoStream;
    AVCodecContext *videoDecoderCtx, *videoEncoderCtx, *audioDecoderCtx;
    AVCodec *videoEncoder_, *videoDecoder, *audioDecoder;
    AVCodecID videoCodec;
    AVAudioResampleContext* resampleCtx;
    int frameFinished;
    uint8_t *output;
    int out_linesize;
    int out_samples;
    int64_t out_sample_fmt;
    ao_sample_format sformat;
    ao_info* info;
    int driver;
    ao_device *adevice;
    AVStream *video_st;
    AVPacket packet;
    AVFrame *audioFrame;
    QImage lastFrame;
    struct SwsContext *imgConvertCtx;
    AVFrame *pFrame;
    AVFrame *pFrame_out;


};

#endif // WATERMARK_H
