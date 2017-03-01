#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videodecoder.h"

#include <QMainWindow>

#include <QFileDialog>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QVideoFrame>
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
    void openFile(QString fileName);
    void convertImageToPixmap(QImage &img,QPixmap &pixmap);

private slots:
    void on_actionAbrir_triggered();
    void displayFrame(QImage image);

private:
    Ui::MainWindow *ui;
    videoDecoder videoDecoder;


};

#endif // MAINWINDOW_H
