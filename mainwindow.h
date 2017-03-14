#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videodecoder.h"
#include "videoencoder.h"
#include "videoinfo.h"

#include <QtSql>

#include <QMainWindow>

#include <QFileDialog>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QImage>
#include <QPainter>
#include <QListWidgetItem>

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
        void play();
        void pause();
        void configureGraphicElements();
        QString printFormatedTime(int64_t time);
        void processVideo();
        void processAudio();
        void prepareVideoConfig();
        void prepareAudioConfig();
        void exportVideo(QString filename, bool vfr);

    private slots:
        void on_stopButton_clicked();
        void on_playPauseButton_clicked();
        void on_getImageButton_clicked();
        void on_listWidget_itemPressed(QListWidgetItem *item);
        void on_playAudioChannelButton_clicked();
        void on_playVideoChannelButton_clicked();
        void on_getInfoButton_clicked();
        void on_openFileButton_clicked();
        void on_exitButton_clicked();
        void on_exportButton_clicked();

private:
        Ui::MainWindow *ui;
        videoDecoder videoDecoder_;
        videoEncoder videoEncoder_;
        bool videoPaused;
        int lastFrameProcessed;
        bool videoStopped;
        QString fileName;
        uint64_t videoDuration;
        bool playAudio;
        bool playVideo;

};

#endif // MAINWINDOW_H
