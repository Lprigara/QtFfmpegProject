#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool openFile(QString fileName);
    void image2Pixmap(QImage &img,QPixmap &pixmap);
    bool isOk();
    void loadVideo(QString fileName);
    bool decodeAndDisplayFrames();
    void displayFrame();
    bool initCodec();
    int getVideoLengthMs();
    void dumpFormat(AVFormatContext *ic, int index, const char *url, int is_output);

private slots:
    void on_actionAbrir_triggered();

private:
    Ui::MainWindow *ui;
    AVFormatContext *formatCtx;
    int videoStream, audioStream, numBytes;
    AVCodecContext *audioCodecCtx, *videoCodecCtx;
    AVCodec *audioCodec, *videoCodec;
    AVFrame *frame, *frameRGB;
    uint8_t *buffer;
    QAudioFormat audioFormat;
    QAudioOutput *audioOutput;
    struct SwsContext *imgConvertCtx;
    bool ok;
    int desiredFrameNumber, desiredFrameTime;
    bool lastFrameOk;
    int lastFrameTime,lastLastFrameTime,lastLastFrameNumber,lastFrameNumber;
    QImage lastFrame;
    AVPacket packet;

};

#endif // MAINWINDOW_H
