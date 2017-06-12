#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "videodecoder.h"
#include "videotranscode.h"
#include "videoremuxing.h"
#include "videocutter.h"
#include "audioextractor.h"
#include "videoscaler.h"
#include "videoextractor.h"
#include "overlaperImage.h"
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

/**
 * @brief Clase encargada de gestionar la interfaz gráfica
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

       /**
         * @brief Inicializa las variables
         */
        void initVariables();

        /**
         * @brief Inicializa los elementos gráficos
         */
        void initGraphicElements();

        /**
         * @brief Carga el contenido multimedia en la aplicación
         */
        void loadMultimediaContent();

        /**
         * @brief Procesa el contenido multimedia
         */
        void processMultimediaContent();

        /**
         * @brief Reproduce el frame
         * @param image
         */
        void displayFrame(QImage image);

        /**
         * @brief Cierra el contenedor de vídeo
         */
        void finishVideo();

        /**
         * @brief Reproduce el contenido multimedia
         */
        void play();

        /**
         * @brief Pausa el contenido
         */
        void pause();

        /**
         * @brief Configura los elementos gráficos
         */
        void configureGraphicElements();

        /**
         * @brief Devuelve el time en formato tiempo (hh:mm:ss:ms)
         * @param time
         * @return
         */
        QString printFormatedTime(int64_t time);

        /**
         * @brief Procesa la pista de vídeo
         */
        void processVideo();

        /**
         * @brief Procesa la pista de audio
         */
        void processAudio();

        /**
         * @brief Configura la pista de vídeo
         */
        void prepareVideoConfig();

        /**
         * @brief Configura la pista de audio
         */
        void prepareAudioConfig();

        /**
         * @brief Setea las etiquetas de los botones de la interfaz
         */
        void fillButtonLabels();
        void displayFrame2(QImage image);
        void process();

    private slots:
        /**
         * @brief Para el vídeo
         */
        void on_stopButton_clicked();

        /**
         * @brief Reproduce o pausa el vídeo
         */
        void on_playPauseButton_clicked();

        /**
         * @brief Obtiene una imagen a partir del vídeo
         */
        void on_getImageButton_clicked();

        /**
         * @brief Reproduce el canal de audio
         */
        void on_playAudioChannelButton_clicked();

        /**
         * @brief Reproduce el canal de vídeo
         */
        void on_playVideoChannelButton_clicked();

        /**
         * @brief Extrae la información del contenido a un fichero
         */
        void on_getInfoButton_clicked();

        /**
         * @brief Abre el fichero
         */
        void on_openFileButton_clicked();

        /**
         * @brief Salir y cerrar la aplicación
         */
        void on_exitButton_clicked();

        /**
         * @brief Cambiar de formato (remuxing)
         */
        void on_formatButton_clicked();

        /**
         * @brief Transcodificar las pistas
         */
        void on_transcodeButton_clicked();

        /**
         * @brief Extraer segmento de vídeo
         */
        void on_cutVideoButton_clicked();

        /**
         * @brief Extraer pista de audio
         */
        void on_extractAudioButton_clicked();

        /**
         * @brief Escalar o redimensionar los frames de la pista de vídeo
         */
        void on_scaleButton_clicked();

        /**
         * @brief Extraer pista de vídeo
         */
        void on_extractVideoButton_clicked();

        /**
         * @brief Guardar en base de datos
         */
        void on_saveInBBDD_clicked();

        /**
         * @brief Superponer imagen a la pista de vídeo
         */
        void on_watermarkButton_clicked();

private:
        Ui::MainWindow *ui;
        videoDecoder videoDecoder_;
        videoTranscode videoEncoder_;
        videoRemuxing videoRemuxing_;
        videoCutter videoCutter_;
        audioExtractor audioExtractor_;
        videoExtractor videoExtractor_;
        videoScaler videoScaler_;
        overlaperImage watermark_;
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
