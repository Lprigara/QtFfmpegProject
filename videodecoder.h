#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QMessageBox>
#include <QDebug>
#include <QFile>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavresample/avresample.h>
    #include <ao/ao.h>
    #include <libavutil/opt.h>
    #include <libavutil/avstring.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/parseutils.h>
    #include <libswscale/swscale.h>
}

/**
 * @brief Clase que permite decodificar las pistas del contenedor para reproducirlas en la interfaz
 */
class videoDecoder : public QObject{
    Q_OBJECT
    public:
        /**
         * @brief Constructor
         * @param parent
         */
        explicit videoDecoder(QObject *parent = 0);
        /**
         * @brief Inicializar variables
         */
        void initVariables();
        /**
         * @brief inicializar códecs
         * @return
         */
        bool initCodec();

        bool loadVideo(QString fileName);
        void decodeFrame(int frameNumber);
        bool readNextFrame();
        void closeVideoAndClean();
        bool getFramesBufferVideo();
        bool findAudioCodec();
        bool findVideoCodec();

        QImage getImage();
        bool isLastFrameOk();
        int64_t getLastFrameTime();
        int64_t getLastSampleTime();
        int64_t getVideoDuration();
        int getLastFrameNumber();
        bool isVideoStream();
        bool isAudioStream();
        bool isOk();
        bool isVideoFinished();
        int getAudioStream();
        int getVideoStream();
        bool hasAudioStream();
        bool hasVideoStream();

        void decodeAndPlayAudioSample();
        void setAudioFormat();
        void checkDelays();

        void getAndSaveInfoInFile(QFile *file);

        QString getVideoCodecInfo();
        QString getAudioCodecInfo();
        QString getDimensions();
        QString getBitrate();
        QString getChannelsNumber();
        QString getVideoCodecName();
        QString getAudioCodecName();

    private:
        AVFormatContext *formatCtx;
        int videoStream, audioStream, numBytes;
        AVCodecContext *audioCodecCtx, *videoCodecCtx;
        AVCodec *audioCodec, *videoCodec;
        AVFrame *frame, *frameRGB;
        AVFrame *audioFrame;
        uint8_t *buffer;
        struct SwsContext *imgConvertCtx;
        bool ok;
        int desiredFrameTime;
        bool lastFrameOk;
        int64_t lastFrameTime;
        int64_t lastSampleTime;
        int lastFrameNumber;
        QImage lastFrame;
        AVPacket packet;
        bool videoFinished;
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
        double lastFrameDelay, lastFramePts;
};

#endif // VIDEODECODE_H
