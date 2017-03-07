#include "videodecoder.h"
#include <QThread>

videoDecoder::videoDecoder(QObject *parent) : QObject(parent)
{
    initCodec();
    initVariables();
}

/***************** INIT VARIABLES AND CODECS *******************************/
void videoDecoder::initVariables(){
    formatCtx = NULL;
    imgConvertCtx = 0;
    videoCodecCtx = 0;
    videoCodec = 0;
    frame = 0;
    frameRGB = 0;
    buffer = 0;
    ok=false;
    videoFinished = false;
    audioFrame = 0;
    resampleCtx = NULL;
    lastFrameDelay=0;
    lastFramePts =0;
}

bool videoDecoder::initCodec()
{
   avcodec_register_all();
   av_register_all();

   printf("License: %s\n",avformat_license());
   printf("AVCodec version %d\n", avformat_version());
   printf("AVFormat configuration: %s\n",avformat_configuration());

   return true;
}
/****************************************************************************/

bool videoDecoder::loadVideo(QString fileName){
    lastFrameTime=0;
    lastFrameNumber=0;
    desiredFrameTime=0;
    lastFrameOk=false;

    // Open video file
    if(avformat_open_input(&formatCtx, fileName.toStdString().c_str(), NULL, NULL)!=0){
        qWarning() << "Couldn't open file";
        return false;
    }

    // Retrieve stream information
    if(avformat_find_stream_info(formatCtx, NULL)<0){
        qWarning() << "Couldn't find stream information";
        return false;
    }

    // Dump information about file onto standard error
    av_dump_format(formatCtx, 0, fileName.toStdString().c_str(), 0);

    videoStream=-1;
    audioStream=-1;
    // Find the video/audio streams
    for(unsigned i=0; i<formatCtx->nb_streams; i++){
        if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoStream=i;
        }
        if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStream=i;
        }
    }
    if(videoStream==-1){
        qWarning() << "Didn't find a video stream";
        return false;
    }
    if(audioStream==-1){
        qWarning() << "Didn't find a audio stream";
        return false;
    }

    ok=true;
    return true;
}

bool videoDecoder::readNextFrame(){
    if(av_read_frame(formatCtx, &packet)>=0)
        return true;
    else{
        videoFinished = true;
        return false;
    }
}


/***************************** AUDIO ****************************************/

bool videoDecoder::findAudioCodec(){
    // Get a pointer to the codec context for the audio stream
    audioCodecCtx=formatCtx->streams[audioStream]->codec;

    //Find the decoder for the audio stream
    audioCodec=avcodec_find_decoder(audioCodecCtx->codec_id);
    if(audioCodec == NULL){
        qWarning()<< "Audio codec not found";
        return false;
    }

    if(avcodec_open2(audioCodecCtx, audioCodec, NULL)<0){
        qWarning() << "Could not open audio codec";
        return false;
    }

    return true;
}

void videoDecoder::setAudioFormat(){
    ao_initialize();

    driver = ao_default_driver_id();
    info = ao_driver_info(driver);

    sformat.bits=16;
    sformat.channels=audioCodecCtx->channels;
    sformat.rate=audioCodecCtx->sample_rate;
    sformat.matrix=0;
    sformat.byte_format=info->preferred_byte_format;

    adevice=ao_open_live(driver,&sformat,NULL);

    if(adevice ==NULL){
        qWarning()<<"Error opening device";
        return;
    }

    if (!(resampleCtx = avresample_alloc_context())) {
       fprintf(stderr, "Could not allocate resample context\n");
       return;
   }

    // The file channels.
    av_opt_set_int(resampleCtx, "in_channel_layout", av_get_default_channel_layout(audioCodecCtx->channels), 0);
    // The device channels.
    av_opt_set_int(resampleCtx, "out_channel_layout",av_get_default_channel_layout(audioCodecCtx->channels), 0);
    // The file sample rate.
    av_opt_set_int(resampleCtx, "in_sample_rate", audioCodecCtx->sample_rate, 0);
    // The device sample rate.
    av_opt_set_int(resampleCtx, "out_sample_rate", audioCodecCtx->sample_rate, 0);
    // The file bit-dpeth.
    av_opt_set_int(resampleCtx, "in_sample_fmt", audioCodecCtx->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    // And now open the resampler. Hopefully all went well.
    if (avresample_open(resampleCtx) < 0) {
        qDebug() << "Could not open resample context.";
        avresample_free(&resampleCtx);
        return;
    }

    avformat_seek_file(formatCtx, 0, 0, 0, 0, 0);

    // We need to use this "getter" for the output sample format.
    av_opt_get_int(resampleCtx, "out_sample_fmt", 0, &out_sample_fmt);

    av_init_packet(&packet);

    audioFrame=av_frame_alloc();

    frameFinished=0;
}


void videoDecoder::decodeAndPlayAudioSample(){

    avcodec_decode_audio4(audioCodecCtx,audioFrame,&frameFinished,&packet);
    if(frameFinished){
        out_samples = avresample_get_out_samples(resampleCtx, audioFrame->nb_samples);

       // Allocate our output buffer.
       av_samples_alloc(&output, &out_linesize, sformat.channels,
               out_samples, (AVSampleFormat)out_sample_fmt, 0);

       // Resample the audio data and store it in our output buffer.
       out_samples = avresample_convert(resampleCtx, &output,
               out_linesize, out_samples, audioFrame->extended_data,
               audioFrame->linesize[0], audioFrame->nb_samples);

       int ret = avresample_available(resampleCtx);
       if (ret)
          fprintf(stderr, "%d converted samples left over\n", ret);

        ao_play(adevice, (char*)output, out_samples*4);
    }
    //free(output);
}

void videoDecoder::checkDelays(){
    int out_delay = avresample_get_delay(resampleCtx);
    while (out_delay) {
        fprintf(stderr, "Flushed %d delayed resampler samples.\n", out_delay);

        // You get rid of the remaining data by "resampling" it with a NULL
        // input.
        out_samples = avresample_get_out_samples(resampleCtx, out_delay);
        av_samples_alloc(&output, &out_linesize, sformat.channels,
                out_delay, (AVSampleFormat)out_sample_fmt, 0);
        out_delay = avresample_convert(resampleCtx, &output, out_linesize,
                out_delay, NULL, 0, 0);
        free(output);
    }
}

/*******************************************************************************/
/***************************** VIDEO *******************************************/
bool videoDecoder::findVideoCodec(){
    // Get a pointer to the codec context for the video stream
    videoCodecCtx=formatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    videoCodec=avcodec_find_decoder(videoCodecCtx->codec_id);
    if(videoCodec==NULL){
        qWarning() << "Video codec not found";
        return false;
    }

    // Open codec
    if(avcodec_open2(videoCodecCtx, videoCodec, NULL)<0){
        qWarning() << "Could not open video codec";
        return false;
    }

    video_st = formatCtx->streams[videoStream];
    return true;
}

bool videoDecoder::getFramesBufferVideo(){

    // Hack to correct wrong frame rates that seem to be generated by some codecs
    if(videoCodecCtx->time_base.num>1000 && videoCodecCtx->time_base.den==1){
        videoCodecCtx->time_base.den=5000;
    }
    // Allocate video frame
    frame=av_frame_alloc();

    // Allocate an AVFrame structure
    frameRGB=av_frame_alloc();
    if(frameRGB==NULL)
        return false;

    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, videoCodecCtx->width,videoCodecCtx->height);
    buffer=new uint8_t[numBytes];

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill((AVPicture *)frameRGB, buffer, AV_PIX_FMT_RGB24,
      videoCodecCtx->width, videoCodecCtx->height);

    return true;
}



void videoDecoder::decodeFrame(int frameNumber){
    int frameFinished1;

    // Is this a packet from the video stream -> decode video frame
    avcodec_decode_video2(videoCodecCtx,frame,&frameFinished1,&packet);

    //Pruebas reproducción slowmotion
    /*double pts = 0, delay;
    pts = av_frame_get_best_effort_timestamp(frame);
    if(pts == AV_NOPTS_VALUE )
        pts = 0;
    pts = av_q2d(video_st->time_base);
    delay = pts - lastFramePts;
    QThread::msleep(delay);*/

    // Did we get a video frame?
    if(frameFinished1){
//        lastFrameDelay = delay;
//        lastFramePts = pts;

       lastFrameNumber=packet.dts;

       // Is this frame the desired frame?
       if(frameNumber==-1 || lastFrameNumber>=frameNumber){
          // It's the desired frame
          // Convert the image format (init the context the first time)
          int width = videoCodecCtx->width;
          int height = videoCodecCtx->height;
          imgConvertCtx = sws_getCachedContext(imgConvertCtx,width, height, videoCodecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

          if(imgConvertCtx == NULL)
          {
             qWarning() <<"Cannot initialize the conversion context!\n";
             return;
          }

          sws_scale(imgConvertCtx, frame->data, frame->linesize, 0, height, frameRGB->data, frameRGB->linesize);

          // Convert the frame to QImage
          lastFrame=QImage(width,height,QImage::Format_RGB888);

          for(int y=0;y<height;y++)
             memcpy(lastFrame.scanLine(y),frameRGB->data[0]+y*frameRGB->linesize[0],width*3);

          // Set the time
         // desiredFrameTime =av_rescale_q(frameNumber,formatCtx->streams[videoStream]->time_base,millisecondbase);
          lastFrameOk=true;
       }
    }  // frameFinished
}

/**********CLEAN AND CLOSE**************/
void videoDecoder::closeVideoAndClean()
{
   // Free the RGB image
   if(buffer)
      delete [] buffer;

   // Free the YUV frame
   if(frame)
      av_free(frame);

   // Free the RGB frame
   if(frameRGB)
      av_free(frameRGB);

   if(audioFrame)
       av_free(audioFrame);

   // Close the codec
   if(videoCodecCtx)
      avcodec_close(videoCodecCtx);

   // Close the video file
   if(formatCtx)
      avformat_close_input(&formatCtx);

   ao_shutdown();

   initVariables();
}

/**********GETTERS Y SETTERS************/

QImage videoDecoder::getImage(){
    return lastFrame;
}

bool videoDecoder::isLastFrameOk(){
    return lastFrameOk;
}

int64_t videoDecoder::getLastFrameTime(){
    AVRational millisecondbase = {1, 1000};
    lastFrameTime= av_rescale_q(packet.dts,formatCtx->streams[videoStream]->time_base,millisecondbase);
    return lastFrameTime;
}

int64_t videoDecoder::getVideoDuration(){
    return formatCtx->duration;
}

int videoDecoder::getLastFrameNumber(){
    return lastFrameNumber;
}

bool videoDecoder::isOk(){
   return ok;
}

bool videoDecoder::isVideoFinished(){
    return videoFinished;
}

bool videoDecoder::isVideoStream(){
    return packet.stream_index==videoStream;
}

bool videoDecoder::isAudioStream(){
    return packet.stream_index==audioStream;
}
