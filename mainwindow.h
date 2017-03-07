#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videodecoder.h"

#include <QMainWindow>

#include <QFileDialog>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QImage>
#include <QPainter>

#include <QMessageBox>

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
    void convertImageToPixmap(QImage &img,QPixmap &pixmap);
    void processVideo();
    void displayFrame(QImage image);
    void finishVideo();
    void initVariables();
    void play();
    void pause();
    QString printFormatedTime(int64_t time);

private slots:
    void on_actionAbrir_triggered();
    void on_stopButton_clicked();
    void on_playPauseButton_clicked();
    void on_getImageButton_clicked();

private:
    Ui::MainWindow *ui;
    videoDecoder videoDecoder_;
    bool videoPaused;
    int lastFrameProcessed;
    bool videoStopped;
    QString fileName;
    uint64_t videoDuration;

signals:
};

#endif // MAINWINDOW_H
