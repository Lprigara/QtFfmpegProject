#include "mainwindow.h"
#include "ui_mainwindow.h"
#define __STDC_CONSTANT_MACROS
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initVariables();
    ui->playPauseButton->setVisible(false);
    ui->stopButton->setVisible(false);
    videoStopped = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initVariables(){
    pause = false;
    lastFrameProcessed = 0;
}

void MainWindow::loadVideo() {
    videoDecoder.closeVideoAndClean();

    videoDecoder.loadVideo(fileName);

    if(videoDecoder.isOk()==false)
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
    processVideo();
}

void MainWindow::displayFrame(QImage image)
{
    if(videoDecoder.isOk()==false){
       QMessageBox::critical(this,"Error","Load a video first");
       return;
    }

   // Convert the QImage to a QPixmap for display
   QPixmap pixmap;
   convertImageToPixmap(image,pixmap);

   // Display the QPixmap
   ui->labelVideoFrame->setPixmap(pixmap);

   // Display the video size
   ui->labelVideoInfo->setText(QString("Display: #%3 @ %4 ms.").arg(videoDecoder.getLastFrameNumber()).arg(videoDecoder.getLastFrameTime()));

   //Repaint
   ui->labelVideoFrame->repaint();
   ui->labelVideoInfo->repaint();

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

    while(pause == false && videoDecoder.readNextFrame()){
        QApplication::processEvents();
        if(videoStopped == true){ return; }
        if(videoDecoder.isVideoStream()){
            videoDecoder.decodeFrame(lastFrameProcessed);
            QImage image = videoDecoder.getFrame();
            displayFrame(image);
        }

        lastFrameProcessed ++;
    }

    if(videoDecoder.isVideoFinished())
       finishVideo();
}

void MainWindow::finishVideo() {
    initVariables();
    ui->playPauseButton->setIcon(QIcon(":playIcon.png"));
    ui->playPauseButton->adjustSize();
    ui->labelVideoFrame->clear();
    ui->labelVideoInfo->clear();
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
        loadVideo();
        return;
    }

    if(pause){
        pause = false;
        ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
        ui->playPauseButton->adjustSize();
        processVideo();
    }else{
        pause = true;
        ui->playPauseButton->setIcon(QIcon(":playIcon.png"));
        ui->playPauseButton->adjustSize();
        return;
    }
}

void MainWindow::on_actionAbrir_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir archivo", QString() ,"Video (*.mjpeg *.mp4 *avi)"); //te devuelve el nombre del archivo
    if (!fileName.isEmpty()){
        this->fileName = fileName;
        loadVideo();
    }
}
