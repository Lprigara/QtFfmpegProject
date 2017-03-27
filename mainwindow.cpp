#include "mainwindow.h"
#include "ui_mainwindow.h"
#define __STDC_CONSTANT_MACROS
#include <QDebug>
#include <QtSql/QtSql>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    initVariables();
    ui->playPauseButton->setVisible(false);
    ui->stopButton->setVisible(false);
    ui->nextFrameButton->setVisible(false);
    ui->previousFrameButton->setVisible(false);
    ui->playVideoChannelButton->setIcon(QIcon(":playIcon.png"));
    ui->playAudioChannelButton->setIcon(QIcon(":playIcon.png"));
    ui->playAudioChannelButton->setVisible(false);
    ui->playVideoChannelButton->setVisible(false);
    ui->getInfoButton->setVisible(false);
    ui->getImageButton->setVisible(false);

    videoStopped = false;

    QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE");
    db.setDatabaseName("FfmpegMediaTool.sqlite");
    if (!db.open()) {
        QMessageBox::critical(NULL, "Error", "No se pudo acceder a los datos.");
    }

    QSqlQuery query;
    query.exec("SELECT * FROM ContenidoMultimedia");
    while(query.next()){
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::DisplayRole, query.value(1).toString());
        item->setData(Qt::UserRole, query.value(2).toString());
        ui->listWidget->addItem(item);
    }

}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::configureGraphicElements(){
    ui->playPauseButton->setVisible(true);
    ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
    ui->playPauseButton->adjustSize();
    ui->stopButton->setVisible(true);
    ui->stopButton->setIcon(QIcon(":stopIcon.png"));
    ui->stopButton->adjustSize();
    ui->nextFrameButton->setVisible(true);
    ui->nextFrameButton->setIcon(QIcon(":nextIcon.png"));
    ui->nextFrameButton->adjustSize();
    ui->previousFrameButton->setVisible(true);
    ui->previousFrameButton->setIcon(QIcon(":previousIcon.png"));
    ui->previousFrameButton->adjustSize();
    ui->displayBar->setMinimum(0);
    ui->displayBar->setMaximum(videoDuration);
    ui->getInfoButton->setVisible(true);
    ui->getImageButton->setVisible(true);
}

void MainWindow::initVariables(){
    videoPaused = false;
    lastFrameProcessed = 0;
}

void MainWindow::loadMultimediaContent() {
    videoDecoder_.closeVideoAndClean();

    videoDecoder_.loadVideo(fileName);
    videoDuration = videoDecoder_.getVideoDuration()/1000;

    if(videoDecoder_.isOk()==false){
       QMessageBox::critical(this,"Error","Error loading the video");
       return;
    }

    configureGraphicElements();


    if(videoDecoder_.getAudioStream() != -1){
        prepareAudioConfig();
    }

    if(videoDecoder_.getVideoStream() != -1){
        prepareVideoConfig();
    }

    processMultimediaContent();
}

void MainWindow::prepareAudioConfig(){
    playAudio = true;
    ui->audioChannelLabel->setText(" Canal de Audio: " + videoDecoder_.getAudioStream());
    ui->playAudioChannelButton->setVisible(true);
    ui->playAudioChannelButton->adjustSize();

    videoDecoder_.findAudioCodec();
    videoDecoder_.setAudioFormat();
}

void MainWindow::prepareVideoConfig(){
    playVideo = true;
    ui->videoChannelLabel->setText("Canal de Vídeo: " + videoDecoder_.getVideoStream());
    ui->playVideoChannelButton->setVisible(true);
    ui->playVideoChannelButton->adjustSize();

    videoDecoder_.findVideoCodec();
    videoDecoder_.getFramesBufferVideo();
}

void MainWindow::displayFrame(QImage image){
    if(videoDecoder_.isOk()==false){
       QMessageBox::critical(this,"Error","Load a video first");
       return;
    }

   // Convert the QImage to a QPixmap for display
   QPixmap pixmap;
   pixmap.convertFromImage(image);
   pixmap = pixmap.scaledToHeight(ui->labelVideoFrame->height());

   // Display the QPixmap
   ui->labelVideoFrame->setAlignment(Qt::AlignCenter);
   ui->labelVideoFrame->setPixmap(pixmap);

   QString timeText = printFormatedTime(videoDecoder_.getLastFrameTime()) + " ----- " + printFormatedTime(videoDuration);
   // Display the video size
   ui->labelVideoInfo->setText(timeText);

   //Repaint
   ui->labelVideoFrame->repaint();
   ui->labelVideoInfo->repaint();

   ui->displayBar->setValue(videoDecoder_.getLastFrameTime());

}

void MainWindow::processMultimediaContent(){

    while(videoPaused == false && videoDecoder_.readNextFrame()){
        QApplication::processEvents();
        if(videoStopped == true){ return; }

        if(playVideo)
            if(videoDecoder_.isVideoStream()){
                processVideo();
            }

        if(playAudio)
            if(videoDecoder_.isAudioStream()){
                processAudio();
            }

        lastFrameProcessed ++;
    }

    videoDecoder_.checkDelays();

    if(videoDecoder_.isVideoFinished())
       finishVideo();
}

void MainWindow::processVideo(){
    videoDecoder_.decodeFrame(lastFrameProcessed);
    QImage image = videoDecoder_.getImage();
    displayFrame(image);
}

void MainWindow::processAudio(){
    videoDecoder_.decodeAndPlayAudioSample();
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


/************************** SLOTS ********************************/
void MainWindow::on_stopButton_clicked()
{
    videoStopped = true;videoStopped = true;
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

void MainWindow::on_openFileButton_clicked()
{
    QStringList routeFiles = QFileDialog::getOpenFileNames(this, "Abrir archivos", QDir::currentPath(), "Video (*.mjpeg *.mp4 *avi *mkv)");
    if( !routeFiles.isEmpty() ){
        for (int i =0;i<routeFiles.count();i++){
            QFileInfo route(routeFiles.at(i));
            QString fileName = route.fileName();
            QString ruta = route.absoluteFilePath();

            QSqlQuery query;
            query.prepare("SELECT * FROM ContenidoMultimedia Where Ruta = :ruta");
            query.bindValue(":ruta", ruta);
            query.exec();

            if(!query.next()){
                QSqlQuery query1;
                query1.prepare("INSERT INTO ContenidoMultimedia(Nombre,Ruta) VALUES(:nombre,:ruta)");
                query1.bindValue(":nombre", fileName);
                query1.bindValue(":ruta", ruta);
                query1.exec();

                QListWidgetItem *item = new QListWidgetItem();
                item->setData(Qt::DisplayRole, fileName);
                item->setData(Qt::UserRole, ruta);
                ui->listWidget->addItem(item);
            }
        }
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
    processMultimediaContent();
}

void MainWindow::on_getImageButton_clicked(){
    pause();
    QString filename = QFileDialog::getSaveFileName(this, "Guardar Imagen", QString(), "Imagen (*.jpg *.png)");
    QImage image = videoDecoder_.getImage();
    image.save(filename);
}

void MainWindow::on_listWidget_itemPressed(QListWidgetItem *item){
    QVariant data = item->data(Qt::UserRole);
    this->fileName = data.toString();
    loadMultimediaContent();
}

void MainWindow::on_playAudioChannelButton_clicked(){
    finishVideo();
    playVideo = false;
    videoStopped = false;
    prepareAudioConfig();
    processMultimediaContent();

}

void MainWindow::on_playVideoChannelButton_clicked(){
    finishVideo();
    playAudio = false;
    videoStopped = false;
    prepareVideoConfig();
    processMultimediaContent();

}

void MainWindow::on_getInfoButton_clicked(){
    QString filename = QFileDialog::getSaveFileName(this, "Guardar Información en fichero", QString(), "Archivo de texto (*.txt)");

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qWarning()<< "Couldn't create file";
        return;
    }

    videoDecoder_.getAndSaveInfoInFile(&file);
}

void MainWindow::on_exitButton_clicked(){
    videoStopped = true;
    qApp->exit();
}

void MainWindow::on_formatButton_clicked(){
    QString srcFile = QFileDialog::getOpenFileName(this, "Archivo origen", QString(), "Video (*.mjpeg *.mp4 *avi *mkv *mov)");
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!srcFile.isNull() && !dstFile.isNull()){
        bool remuxingOk = videoEncoder_.remuxing(srcFile.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(remuxingOk){
            QMessageBox::information(this, "Info", "El archivo ha cambiado de formato correctamente");
        }else{
            QMessageBox::critical(this, "Error", "El archivo no se ha podido cambiar de formato");
        }
    }
}

void MainWindow::on_transcodeButton_clicked(){
    QString srcFile = QFileDialog::getOpenFileName(this, "Archivo origen", QString(), "Video (*.mjpeg *.mp4 *avi *mkv *mov)");
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!srcFile.isNull() && !dstFile.isNull()){
        QString audioCodec = ui->audioCodecSelector->currentText();
        QString videoCodec = ui->videoCodecSelector->currentText();
        bool transcodeOk = videoEncoder_.transcode(srcFile.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData(), audioCodec, videoCodec);
        if(transcodeOk){
            QMessageBox::information(this, "Info", "El archivo ha cambiado de códecs correctamente");
        }else{
            QMessageBox::critical(this, "Error", "El archivo no se ha podido cambiar de códecs");
        }
    }
}

void MainWindow::on_cutVideoButton_clicked(){
    QString srcFile = QFileDialog::getOpenFileName(this, "Archivo origen", QString(), "Video (*.mjpeg *.mp4 *avi *mkv *mov)");
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!srcFile.isNull() && !dstFile.isNull()){
        double startVideo = ui->startVideoSpinBox->value();
        double finishVideo = ui->finishVideoSpinBox->value();

        bool cutOk = videoCutter_.cutVideo(startVideo, finishVideo, srcFile.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(cutOk){
            QMessageBox::information(this, "Info", "La segmentación del vídeo ha sido satisfactoria");
        }else{
            QMessageBox::critical(this, "Error", "Ha ocurrido un error al segmentar el vídeo");
        }
    }
}
