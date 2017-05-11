#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videodecoder.h"
#include "videoencoder.h"
#include "videoinfo.h"
#include "videocutter.h"
#include "audioextractor.h"
#include "videoscaler.h"
#include "audiofilter.h"
#include "videoextractor.h"
#include "watermark.h"
#include <QtSql>

#include <QMainWindow>

#include <QFileDialog>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QImage>
#include <QPainter>

#include <QMessageBox>
#include <QFileInfo>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();
        void loadMultimediaContent();
        void processMultimediaContent();
        void displayFrame(QImage image);
        void finishVideo();
        void initVariables();
        void initGraphicElements();
        void play();
        void pause();
        void configureGraphicElements();
        QString printFormatedTime(int64_t time);
        void processVideo();
        void processAudio();
        void prepareVideoConfig();
        void prepareAudioConfig();
        void exportVideo(QString filename, bool vfr);
        void loadInformation();
        void fillButtonLabels();
        void displayFrame2(QImage image);
        void process();

    private slots:
        void on_stopButton_clicked();
        void on_playPauseButton_clicked();
        void on_getImageButton_clicked();
        void on_playAudioChannelButton_clicked();
        void on_playVideoChannelButton_clicked();
        void on_getInfoButton_clicked();
        void on_openFileButton_clicked();
        void on_exitButton_clicked();
        void on_formatButton_clicked();
        void on_transcodeButton_clicked();
        void on_cutVideoButton_clicked();
        void on_extractAudioButton_clicked();
        void on_scaleButton_clicked();
        void on_extractVideoButton_clicked();
        void on_saveInBBDD_clicked();
        void on_watermarkButton_clicked();

private:
        Ui::MainWindow *ui;
        videoDecoder videoDecoder_;
        videoEncoder videoEncoder_;
        videoCutter videoCutter_;
        audioExtractor audioExtractor_;
        videoExtractor videoExtractor_;
        videoScaler videoScaler_;
        audioFilter audioFilter_;
        watermark watermark_;
        bool videoPaused;
        int lastFrameProcessed;
        bool videoStopped;
        QString fileName;
        QString ruta;
        uint64_t videoDuration;
        bool playAudio;
        bool playVideo;

};

#endif // MAINWINDOW_H
