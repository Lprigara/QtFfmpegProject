#include "mainwindow.h"
#include "ui_mainwindow.h"
#define __STDC_CONSTANT_MACROS
#include <QDebug>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initVariables();
    ui->playPauseButton->setVisible(false);
    ui->stopButton->setVisible(false);
    videoStopped = false;

    connect(&videoDecoder_, SIGNAL(sleepe(double)), this, SLOT(sleep(double)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initVariables(){
    videoPaused = false;
    lastFrameProcessed = 0;
}

void MainWindow::loadMultimediaContent() {
    videoDecoder_.closeVideoAndClean();

    videoDecoder_.loadVideo(fileName);
    videoDuration = videoDecoder_.getVideoDuration()/1000;

    if(videoDecoder_.isOk()==false)
    {
       QMessageBox::critical(this,"Error","Error loading the video");
       return;
    }

    ui->playPauseButton->setVisible(true);
    ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
    ui->playPauseButton->adjustSize();
    ui->stopButton->setVisible(true);
    ui->stopButton->setIcon(QIcon(":stopIcon.png"));
    ui->stopButton->adjustSize();
    ui->displayBar->setMinimum(0);
    ui->displayBar->setMaximum(videoDuration);

    videoDecoder_.findAudioCodec();
    videoDecoder_.setAudioFormat();
    videoDecoder_.findVideoCodec();
    videoDecoder_.getFramesBufferVideo();

    processVideo();
}

void MainWindow::displayFrame(QImage image)
{
    if(videoDecoder_.isOk()==false){
       QMessageBox::critical(this,"Error","Load a video first");
       return;
    }

   // Convert the QImage to a QPixmap for display
   QPixmap pixmap;
   convertImageToPixmap(image,pixmap);

   // Display the QPixmap
   ui->labelVideoFrame->setPixmap(pixmap);

   QString timeText = printFormatedTime(videoDecoder_.getLastFrameTime()) + " ----- " + printFormatedTime(videoDuration);
   // Display the video size
   ui->labelVideoInfo->setText(timeText);

   //Repaint
   ui->labelVideoFrame->repaint();
   ui->labelVideoInfo->repaint();

   ui->displayBar->setValue(videoDecoder_.getLastFrameTime());

}

void MainWindow::convertImageToPixmap(QImage &image,QPixmap &pixmap)
{
   // Convert the QImage to a QPixmap for display
   pixmap = QPixmap(image.size());
   QPainter painter;
   painter.begin(&pixmap);
   painter.drawImage(0,0,image);
   painter.end();
}

void MainWindow::processVideo(){

    while(videoPaused == false && videoDecoder_.readNextFrame()){
        QApplication::processEvents();
        if(videoStopped == true){ return; }

        if(videoDecoder_.isVideoStream()){
            videoDecoder_.decodeFrame(lastFrameProcessed);
            QImage image = videoDecoder_.getImage();
            displayFrame(image);
        }

        if(videoDecoder_.isAudioStream()){
            videoDecoder_.decodeAndPlayAudioSample();
        }

        lastFrameProcessed ++;
    }

    videoDecoder_.checkDelays();

    if(videoDecoder_.isVideoFinished())
       finishVideo();
}

void MainWindow::finishVideo() {
    initVariables();
    ui->playPauseButton->setIcon(QIcon(":playIcon.png"));
    ui->playPauseButton->adjustSize();
    ui->labelVideoFrame->clear();
    ui->labelVideoInfo->clear();
    ui->displayBar->setValue(0);
    videoStopped = true;
}

/************************** SLOTS ********************************/
void MainWindow::on_stopButton_clicked()
{
    videoStopped = true;
    finishVideo();
}

void MainWindow::on_playPauseButton_clicked()
{
    if(videoStopped == true){
        videoStopped = false;
        ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
        ui->playPauseButton->adjustSize();
        loadMultimediaContent();
        return;
    }

    if(videoPaused){
        play();
    }else{
        pause();
    }
}

void MainWindow::on_actionAbrir_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir archivo", QString() ,"Video (*.mjpeg *.mp4 *avi)"); //te devuelve el nombre del archivo
    if (!fileName.isEmpty()){
        this->fileName = fileName;
        loadMultimediaContent();
    }
}

void MainWindow::pause(){
    videoPaused = true;
    ui->playPauseButton->setIcon(QIcon(":playIcon.png"));
    ui->playPauseButton->adjustSize();
}

void MainWindow::play(){
    videoPaused = false;
    ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
    ui->playPauseButton->adjustSize();
    processVideo();
}

void MainWindow::on_getImageButton_clicked()
{
    pause();
    QString filename = QFileDialog::getSaveFileName(this, "Guardar Imagen", QString(), "Imagen (*.jpg *.png)");
    QImage image = videoDecoder_.getImage();
    image.save(filename);
}

QString MainWindow::printFormatedTime(int64_t time){
    int hours, mins, secs;
    secs = time / 1000;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;

    QString timeTextFormat = "";
    if(hours/10 < 1 && hours !=10)
        timeTextFormat = "0%2:";
    else
        timeTextFormat = "%2:";
    if(mins/10 <1 && mins != 10)
        timeTextFormat += "0%3:";
    else
        timeTextFormat += "%3:";
    if(secs/10 <1 && secs != 10)
        timeTextFormat += "0%4";
    else
        timeTextFormat += "%4";

    return timeTextFormat.arg(hours).arg(mins).arg(secs);
}
