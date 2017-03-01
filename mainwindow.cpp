#include "mainwindow.h"
#include "ui_mainwindow.h"
#define __STDC_CONSTANT_MACROS
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //objeto que emite la señal, señal emitida, objeto que recibe la señal, accion que desencadena esa señal
    connect(&videoDecoder, SIGNAL(signalDisplayFrame(QImage)), this, SLOT(displayFrame(QImage)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAbrir_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir archivo", QString() ,"Video (*.mjpeg *.mp4 *avi)"); //te devuelve el nombre del archivo
    if (!fileName.isEmpty())
        openFile(fileName);
}

void MainWindow::openFile(QString fileName) {

    videoDecoder.loadVideo(fileName);

    if(videoDecoder.isOk()==false)
    {
       QMessageBox::critical(this,"Error","Error loading the video");
       return;
    }

    videoDecoder.decodeAndDisplayFrames();
}

void MainWindow::displayFrame(QImage image)
{
    if(videoDecoder.isOk()==false){
       QMessageBox::critical(this,"Error","Load a video first");
       return;
    }

   // Decode a frame
   if(!videoDecoder.isLastFrameOk()){
      QMessageBox::critical(this,"Error","Error decoding the frame");
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

void MainWindow::convertImageToPixmap(QImage &img,QPixmap &pixmap)
{
   // Convert the QImage to a QPixmap for display
   pixmap = QPixmap(img.size());
   QPainter painter;
   painter.begin(&pixmap);
   painter.drawImage(0,0,img);
   painter.end();
}
