#include "mainwindow.h"
#include "ui_mainwindow.h"
#define __STDC_CONSTANT_MACROS
#include <QDebug>
#include <QtSql/QtSql>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    initVariables();
    initGraphicElements();

    videoStopped = false;

    QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE");
    db.setDatabaseName("FfmpegMediaTool.sqlite");
    if (!db.open()) {
        QMessageBox::critical(NULL, "Error", "No se pudo acceder a los datos.");
    }
}

MainWindow::~MainWindow(){
    delete ui;
}

//Función que inicializa los elementos gráficos de la interfaz
void MainWindow::initGraphicElements(){
    ui->playPauseButton->setDisabled(true);
    ui->stopButton->setDisabled(true);
    ui->nextFrameButton->setDisabled(true);
    ui->previousFrameButton->setDisabled(true);
    ui->playVideoChannelButton->setIcon(QIcon(":playIcon.png"));
    ui->playAudioChannelButton->setIcon(QIcon(":playIcon.png"));
    ui->playAudioChannelButton->setDisabled(true);
    ui->playVideoChannelButton->setDisabled(true);
    ui->getInfoButton->setDisabled(true);
    ui->getImageButton->setDisabled(true);
    ui->audioChannelLabel->setText("Canal de Audio: ");
    ui->videoChannelLabel->setText("Canal de Vídeo: ");
    ui->formatButton->setDisabled(true);
    ui->transcodeButton->setDisabled(true);
    ui->scaleButton->setDisabled(true);
    ui->cutVideoButton->setDisabled(true);
    ui->extractAudioButton->setDisabled(true);
    ui->extractVideoButton->setDisabled(true);
}

//Función que configura los elementos gráficos de la interfaz
void MainWindow::configureGraphicElements(){
    ui->playPauseButton->setDisabled(false);
    ui->playPauseButton->setIcon(QIcon(":pauseIcon.png"));
    ui->playPauseButton->adjustSize();
    ui->stopButton->setDisabled(false);
    ui->stopButton->setIcon(QIcon(":stopIcon.png"));
    ui->stopButton->adjustSize();
    ui->nextFrameButton->setDisabled(false);
    ui->nextFrameButton->setIcon(QIcon(":nextIcon.png"));
    ui->nextFrameButton->adjustSize();
    ui->previousFrameButton->setDisabled(false);
    ui->previousFrameButton->setIcon(QIcon(":previousIcon.png"));
    ui->previousFrameButton->adjustSize();
    ui->displayBar->setMinimum(0);
    ui->displayBar->setMaximum(videoDuration);
    ui->getInfoButton->setDisabled(false);
    ui->formatButton->setDisabled(false);
    ui->transcodeButton->setDisabled(false);
    ui->cutVideoButton->setDisabled(false);

    if(videoDecoder_.hasAudioStream()){
        ui->extractAudioButton->setDisabled(false);
    }
    if(videoDecoder_.hasVideoStream()){
        ui->extractVideoButton->setDisabled(false);
        ui->scaleButton->setDisabled(false);
        ui->getImageButton->setDisabled(false);
    }
}

//Función que setea las etiquetas de información de la interfaz
void MainWindow::fillButtonLabels(){
    ui->labelFilename->setText(fileName);
    ui->labelDuration->setText(printFormatedTime(videoDuration));
    ui->labelbitrate->setText(videoDecoder_.getBitrate());
    ui->labelChannelsNumber->setText(videoDecoder_.getChannelsNumber());
    ui->labelCurrentDuration->setText(printFormatedTime(videoDuration));

    if(videoDecoder_.hasAudioStream()){
        ui->labelAudioCodec->setText(videoDecoder_.getAudioCodecInfo());
        ui->labelCurrentAudioCodec->setText(videoDecoder_.getAudioCodecName());
        ui->LabelInfoAudio->setText(videoDecoder_.getAudioCodecInfo());
    }

    if(videoDecoder_.hasVideoStream()){
        ui->labelVideooCodec->setText(videoDecoder_.getVideoCodecInfo());
        ui->labelDimensions->setText(videoDecoder_.getDimensions());
        ui->labelCurrentDimensions->setText(videoDecoder_.getDimensions());
        ui->labelCurrentVideoCodec->setText(videoDecoder_.getVideoCodecName());
        ui->LabelInfoVideo->setText(videoDecoder_.getVideoCodecInfo());
    }

    int lastPoint = fileName.lastIndexOf(".") + 1;
    QString extension = fileName.right(fileName.count() - lastPoint);
    ui->labelCurrentFormat->setText(extension.toUpper());
}

void MainWindow::initVariables(){
    videoPaused = false;
    lastFrameProcessed = 0;
}

void MainWindow::loadMultimediaContent() {
    videoDecoder_.closeVideoAndClean();

    videoDecoder_.loadVideo(ruta);
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

    fillButtonLabels();

    processMultimediaContent();
}

void MainWindow::prepareAudioConfig(){
    playAudio = true;
    ui->audioChannelLabel->setText(" Canal de Audio: " + videoDecoder_.getAudioStream());
    ui->playAudioChannelButton->setDisabled(false);
    ui->playAudioChannelButton->adjustSize();

    videoDecoder_.findAudioCodec();
    videoDecoder_.setAudioFormat();
}

void MainWindow::prepareVideoConfig(){
    playVideo = true;
    ui->videoChannelLabel->setText("Canal de Vídeo: " + videoDecoder_.getVideoStream());
    ui->playVideoChannelButton->setDisabled(false);
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
    if(!videoDecoder_.isVideoStream()){
        QString timeText = printFormatedTime(videoDecoder_.getLastSampleTime()) + " ----- " + printFormatedTime(videoDuration);
        // Display the video size
        ui->labelVideoInfo->setText(timeText);
        ui->labelVideoInfo->repaint();
        ui->displayBar->setValue(videoDecoder_.getLastSampleTime());
    }
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
    QString routeFile = QFileDialog::getOpenFileName(this, "Abrir archivo", QDir::currentPath(), "Video (*.mjpeg *.mp4 *avi *mkv *mov *mp3)");
    if( !routeFile.isEmpty() ){
        QFileInfo route(routeFile);
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
        }

        this->fileName = fileName;
        this->ruta = ruta;

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
    processMultimediaContent();
}

//Función que permite extraer un frame del vídeo y guardarla como imagen
void MainWindow::on_getImageButton_clicked(){
    pause();
    QString filename = QFileDialog::getSaveFileName(this, "Guardar Imagen", QString(), "Imagen (*.jpg *.png)");
    QImage image = videoDecoder_.getImage();
    image.save(filename);
}

//Función que permite reproducir solamente el canal de audio
void MainWindow::on_playAudioChannelButton_clicked(){
    finishVideo();
    playVideo = false;
    videoStopped = false;
    prepareAudioConfig();
    processMultimediaContent();

}

//Función que permite reproducir solamente el canal de vídeo
void MainWindow::on_playVideoChannelButton_clicked(){
    finishVideo();
    playAudio = false;
    videoStopped = false;
    prepareVideoConfig();
    processMultimediaContent();

}

//Función que llama a la clase videoDecoder que permite extraer información del contenido multimedia y almacenarla en un fichero
void MainWindow::on_getInfoButton_clicked(){
    QString filename = QFileDialog::getSaveFileName(this, "Guardar Información en fichero", QString(), "Archivo de texto (*.txt)");

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qWarning()<< "Couldn't create file";
        return;
    }

    videoDecoder_.getAndSaveInfoInFile(&file);
}

//Función que llama a la clase videoEncoder que permite cambiar de formato el archivo multimedia
void MainWindow::on_formatButton_clicked(){
    QString formatDst = ui->formatSelector->currentText();
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), formatDst);

    //Mejorar esta parte. Lo que hace es quitar la extensión del archivo y añadirle la elegida en el selector
    int lastPoint = dstFile.lastIndexOf(".");
    dstFile = dstFile.left(lastPoint);
    dstFile = dstFile.append("." + formatDst.toLower());

    if(!ruta.isNull() && !dstFile.isNull()){
        bool remuxingOk = videoEncoder_.remuxing(ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(remuxingOk){
            QMessageBox::information(this, "Info", "El archivo ha cambiado de formato correctamente");
        }else{
            QMessageBox::critical(this, "Error", "El archivo no se ha podido cambiar de formato");
        }
    }
}

//Función que llama a la clase videoEncoder que permite transcodificar tanto la pista de vídeo como la de auidio y almacenarlas en un archivo destino
void MainWindow::on_transcodeButton_clicked(){
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!ruta.isNull() && !dstFile.isNull()){
        QString audioCodec = ui->audioCodecSelector->currentText();
        QString videoCodec = ui->videoCodecSelector->currentText();
        bool transcodeOk = videoEncoder_.transcode(ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData(), audioCodec, videoCodec);
        if(transcodeOk){
            QMessageBox::information(this, "Info", "El archivo ha cambiado de códecs correctamente");
        }else{
            QMessageBox::critical(this, "Error", "El archivo no se ha podido cambiar de códecs");
        }
    }
}

//Función que llama a la clase videoCutter que permite extraer un fragmento del vídeo a través de dos marcas de tiempo y almacenarla en un archivo destino
void MainWindow::on_cutVideoButton_clicked(){
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!ruta.isNull() && !dstFile.isNull()){
        double startVideo = ui->startVideoSpinBox->value();
        double finishVideo = ui->finishVideoSpinBox->value();

        bool cutOk = videoCutter_.cutVideo(startVideo, finishVideo, ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(cutOk){
            QMessageBox::information(this, "Info", "La segmentación del vídeo ha sido satisfactoria");
        }else{
            QMessageBox::critical(this, "Error", "Ha ocurrido un error al segmentar el vídeo");
        }
    }
}


//Función que llama a la clase audioExtractor que permite extraer la pista de audio y almacenarla en un archivo destino
void MainWindow::on_extractAudioButton_clicked(){
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    if(!ruta.isNull() && !dstFile.isNull()){
        bool extractAudioOk = audioExtractor_.convert(ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(extractAudioOk){
            QMessageBox::information(this, "Info", "La extracción del audio ha sido satisfactoria");
        }else{
            QMessageBox::critical(this, "Error", "Ha ocurrido un error al extraer el audio");
        }
    }
}

//Función que llama a la clase videoExtractor que permite extraer la pista de vídeo y almacenarla en un archivo destino
void MainWindow::on_extractVideoButton_clicked(){
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    int lastPoint = dstFile.lastIndexOf(".");
    dstFile = dstFile.left(lastPoint);
    dstFile = dstFile.append(".mkv");
    if(!ruta.isNull() && !dstFile.isNull()){
        bool extractAudioOk = videoExtractor_.convert(ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData());
        if(extractAudioOk){
            QMessageBox::information(this, "Info", "La extracción del vídeo ha sido satisfactoria");
        }else{
            QMessageBox::critical(this, "Error", "Ha ocurrido un error al extraer el vídeo");
        }
    }
}


//Función que llama a la clase videoScaler que permite cambiar la escala del vídeo
void MainWindow::on_scaleButton_clicked(){
    QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    int height = ui->height->value();
    int width = ui->width->value();
    if(!ruta.isNull() && !dstFile.isNull()){
        bool videoScalerOk = videoScaler_.processScaled(ruta.toLocal8Bit().constData(), dstFile.toLocal8Bit().constData(), height, width);
        if(videoScalerOk){
            QMessageBox::information(this, "Info", "El vídeo ha sido escalado satisfactoriamente");
        }else{
            QMessageBox::critical(this, "Error", "Ha ocurrido un error al escalar el vídeo");
        }
    }
}

void MainWindow::on_exitButton_clicked(){
    videoStopped = true;
    qApp->exit();
}

void MainWindow::on_saveInBBDD_clicked(){
    QSqlQuery query;
    query.prepare("UPDATE ContenidoMultimedia SET Autor = :autor, Genero = :genero,"
                   "Descripcion = :descripcion, Comentarios = :comentarios where Ruta = :ruta");

    query.bindValue(":autor", ui->autorText->toPlainText());
    query.bindValue(":genero", ui->generoText->toPlainText());
    query.bindValue(":descripcion", ui->descripcionText->toPlainText());
    query.bindValue(":comentarios", ui->comentariosText->toPlainText());
    query.bindValue(":ruta", ruta);
    query.exec();
}

void MainWindow::on_watermarkButton_clicked(){
    //QString in = QFileDialog::getOpenFileName(this, "Archivo", QDir::currentPath(), "Video (*.mp4)");

    QString watermark = QFileDialog::getOpenFileName(this, "Abrir marca de agua", QDir::currentPath(), "Imagen (*.png)");
    //QString dstFile = QFileDialog::getSaveFileName(this, "Archivo destino", QString(), "Video");
    QApplication::processEvents();

    watermark_.main(ruta.toLocal8Bit().constData(), watermark.toLocal8Bit().constData(), "");
    process();

}

void MainWindow::process(){

    while(watermark_.readNextFrame()){
        QApplication::processEvents();
        if(watermark_.isVideoStream()){
            watermark_.decodeVideoFrame();
            QImage imagen = watermark_.getImage();
            displayFrame2(imagen);
        }
        if(watermark_.isAudioStream()){
            watermark_.decodeAudioFrame();
        }
        watermark_.checkDelays();
    }
}


void MainWindow::displayFrame2(QImage image){
   // Convert the QImage to a QPixmap for display
   QPixmap pixmap;
   pixmap.convertFromImage(image);
   pixmap = pixmap.scaledToHeight(ui->labelVideoFrame->height());

   // Display the QPixmap
   ui->labelVideoFrame->setAlignment(Qt::AlignCenter);
   ui->labelVideoFrame->setPixmap(pixmap);

   //QString timeText = printFormatedTime(videoDecoder_.getLastFrameTime()) + " ----- " + printFormatedTime(videoDuration);
   // Display the video size
   //ui->labelVideoInfo->setText(timeText);

   //Repaint
   ui->labelVideoFrame->repaint();
   //ui->labelVideoInfo->repaint();

   //ui->displayBar->setValue(videoDecoder_.getLastFrameTime());

}
