#include "watermark.h"
#include <QDebug>


watermark::watermark()
{
    imgConvertCtx = 0;
    pFormatCtx = NULL;
    outFormatCtx=NULL;
}

int watermark::open_input_file(const char *filename){
    int ret;

    if ((ret = avformat_open_input(&pFormatCtx, filename, NULL, NULL)) < 0) {
        qInfo()<<"Cannot open input file\n";
        return ret;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        qDebug()<<"Cannot find stream information";
        return false;
    }

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            videoDecoderCtx = pFormatCtx->streams[i]->codec;
            videoDecoder =  avcodec_find_decoder(videoDecoderCtx->codec_id);
            if(videoDecoder == NULL){
                qDebug() << "Codec not found";
                return false;
            }

            if(avcodec_open2(videoDecoderCtx, videoDecoder, NULL) <0){
                qDebug() <<"Failed to open video decoder";
                return false;
            }
        }else if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
            audioDecoderCtx = pFormatCtx->streams[i]->codec;
            audioDecoder = avcodec_find_decoder(audioDecoderCtx->codec_id);
            if(audioDecoder == NULL){
                qDebug()<< "Codec not found";
                return false;
            }
            if(avcodec_open2(audioDecoderCtx, audioDecoder, NULL) <0){
                qDebug() <<"Failed to open audio decoder";
                return false;
            }
        }
    }

    return 0;
}

int watermark::init_filters(const char *filters_descr)
{
    char args[512];
    int ret;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            videoDecoderCtx->width, videoDecoderCtx->height, videoDecoderCtx->pix_fmt,
            videoDecoderCtx->time_base.num, videoDecoderCtx->time_base.den,
            videoDecoderCtx->sample_aspect_ratio.num, videoDecoderCtx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        qInfo()<<("Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        qInfo()<<("Cannot create buffer sink\n");
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        return ret;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;
    return 0;
}

bool watermark::convertImage(AVFrame frame){

    // Allocate an AVFrame structure
    AVFrame *frameRGB=av_frame_alloc();
    if(frameRGB==NULL)
        return false;

    // Determine required buffer size and allocate buffer
    int numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, videoDecoderCtx->width,videoDecoderCtx->height);
    uint8_t *buffer=new uint8_t[numBytes];

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill((AVPicture *)frameRGB, buffer, AV_PIX_FMT_RGB24,
      videoDecoderCtx->width, videoDecoderCtx->height);

    int width = videoDecoderCtx->width;
    int height = videoDecoderCtx->height;
    imgConvertCtx = sws_getCachedContext(imgConvertCtx,width, height, videoDecoderCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    if(imgConvertCtx == NULL)
    {
       qWarning() <<"Cannot initialize the conversion context!\n";
       return -1;
    }

    sws_scale(imgConvertCtx, frame.data, frame.linesize, 0, height, frameRGB->data, frameRGB->linesize);

    // Convert the frame to QImage
    lastFrame=QImage(width,height,QImage::Format_RGB888);

    for(int y=0;y<height;y++)
       memcpy(lastFrame.scanLine(y),frameRGB->data[0]+y*frameRGB->linesize[0],width*3);
}


int watermark::main(const char* inFilename, const char *waterMark, const char* outFilename)
{
    int ret;
    av_register_all();
    avfilter_register_all();

    QString filter_descr = "movie=";
    filter_descr.append(waterMark);
    filter_descr.append("[wm];[in][wm]overlay=5:5[out]");

    if ((ret = open_input_file(inFilename)) < 0)
        return -1;
    if ((ret = init_filters(filter_descr.toLocal8Bit().constData())) < 0)
        return -1;


    pFrame=av_frame_alloc();
    pFrame_out=av_frame_alloc();

    setAudioFormat();

}

void watermark::decodeVideoFrame(){
    av_frame_unref(pFrame);
    int got_frame = 0;
    if(avcodec_decode_video2(videoDecoderCtx, pFrame, &got_frame, &packet) < 0) {
        qInfo()<<( "Error decoding video\n");
        return;
    }

    if (got_frame) {
        pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

        /* push the decoded frame into the filtergraph */
        if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
            qInfo()<<( "Error while feeding the filtergraph\n");
            return;
        }


        int ret = av_buffersink_get_frame(buffersink_ctx, pFrame_out);
        if (ret < 0)
            return;

        convertImage(*pFrame_out);

    }
}

void watermark::decodeAudioFrame(){
    av_frame_unref(pFrame);
    int got_frame = 0;

    if(avcodec_decode_audio4(audioDecoderCtx, pFrame, &got_frame, &packet) < 0) {
        qInfo()<<( "Error decoding audio\n");
        return;
    }

    if (got_frame) {
       decodeAndPlayAudioSample();
    }
}


bool watermark::readNextFrame(){
    if(av_read_frame(pFormatCtx, &packet)>=0)
        return true;
    else
        return false;
}

bool watermark::isVideoStream(){
    return packet.stream_index==video_stream_index;
}

bool watermark::isAudioStream(){
    return packet.stream_index==audio_stream_index;
}

QImage watermark::getImage(){
    return lastFrame;
}

void watermark::decodeAndPlayAudioSample(){

    avcodec_decode_audio4(audioDecoderCtx,audioFrame,&frameFinished,&packet);
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

void watermark::checkDelays(){
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
void watermark::setAudioFormat(){
    ao_initialize();

    driver = ao_default_driver_id();
    info = ao_driver_info(driver);

    sformat.bits=16;
    sformat.channels=audioDecoderCtx->channels;
    sformat.rate=audioDecoderCtx->sample_rate;
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
    av_opt_set_int(resampleCtx, "in_channel_layout", av_get_default_channel_layout(audioDecoderCtx->channels), 0);
    // The device channels.
    av_opt_set_int(resampleCtx, "out_channel_layout",av_get_default_channel_layout(audioDecoderCtx->channels), 0);
    // The file sample rate.
    av_opt_set_int(resampleCtx, "in_sample_rate", audioDecoderCtx->sample_rate, 0);
    // The device sample rate.
    av_opt_set_int(resampleCtx, "out_sample_rate", audioDecoderCtx->sample_rate, 0);
    // The file bit-dpeth.
    av_opt_set_int(resampleCtx, "in_sample_fmt", audioDecoderCtx->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    // And now open the resampler. Hopefully all went well.
    if (avresample_open(resampleCtx) < 0) {
        qDebug() << "Could not open resample context.";
        avresample_free(&resampleCtx);
        return;
    }

    avformat_seek_file(pFormatCtx, 0, 0, 0, 0, 0);

    // We need to use this "getter" for the output sample format.
    av_opt_get_int(resampleCtx, "out_sample_fmt", 0, &out_sample_fmt);

    av_init_packet(&packet);

    audioFrame=av_frame_alloc();

    frameFinished=0;
}
