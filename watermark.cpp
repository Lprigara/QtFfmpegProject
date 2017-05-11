#include "watermark.h"
#include <QDebug>

#define DISPLAY 1
#define ENCODE 0

watermark::watermark(){
    pFormatCtx = NULL;
    outFormatCtx=NULL;
}

int watermark::open_input_file(const char *filename){
    int ret;
    AVCodec *dec;

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
        }
        else if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
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

 /*   if ((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        qInfo()<< "Cannot find stream information\n";
        return ret;
    }

    // select the video stream
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        qInfo()<< "Cannot find a video stream in the input file\n";
        return ret;
    }
    video_stream_index = ret;
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;

    // init the video decoder
    if ((ret = avcodec_open2(pCodecCtx, dec, NULL)) < 0) {
        qInfo()<< "Cannot open video decoder\n";
        return ret;
    }*/

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


int watermark::main(const char* inFilename, const char *waterMark, const char* outFilename)
{
    int ret;
    AVFrame *pFrame;
    AVFrame *pFrame_out;

    int got_frame;

    av_register_all();
    avfilter_register_all();

    QString filter_descr = "movie=";
    filter_descr.append(waterMark);
    filter_descr.append("[wm];[in][wm]overlay=5:5[out]");

    if ((ret = open_input_file(inFilename)) < 0)
        goto end;
    if (( ret = openOutputFile(outFilename)) < 0)
        goto end;
    if ((ret = init_filters(filter_descr.toLocal8Bit().constData())) < 0)
        goto end;

    setAudioFormat();

    SDL_Surface *screen;
    SDL_Overlay *bmp;
    SDL_Rect rect;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        qInfo()<<( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    screen = SDL_SetVideoMode(videoDecoderCtx->width, videoDecoderCtx->height, 0, 0);
    if(!screen) {
        qInfo()<<("SDL: could not set video mode - exiting\n");
        return -1;
    }
    bmp = SDL_CreateYUVOverlay(videoDecoderCtx->width, videoDecoderCtx->height,SDL_YV12_OVERLAY, screen);

    SDL_WM_SetCaption("Simplest FFmpeg Video Filter",NULL);


    pFrame=av_frame_alloc();
    pFrame_out=av_frame_alloc();

    /* read all packets */
    while ((av_read_frame(pFormatCtx, &packet)) >= 0) {
        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_video2(videoDecoderCtx, pFrame, &got_frame, &packet);
            if (ret < 0) {
                qInfo()<<( "Error decoding video\n");
                break;
            }

            if (got_frame) {
                pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
                    qInfo()<<( "Error while feeding the filtergraph\n");
                    break;
                }


                ret = av_buffersink_get_frame(buffersink_ctx, pFrame_out);
                if (ret < 0)
                    break;

                qInfo()<<("Process 1 frame!\n");

                //CODIFICAR PARA ENVIAR A SALIDA
                if(ENCODE)
                encodeWriteFrame(pFrame_out, packet.stream_index);

                //REPRODUCIR
                else if(DISPLAY){
                    if (pFrame_out->format==AV_PIX_FMT_YUV420P) {

                        SDL_LockYUVOverlay(bmp);
                        int y_size=pFrame_out->width*pFrame_out->height;
                        memcpy(bmp->pixels[0],pFrame_out->data[0],y_size);   //Y
                        memcpy(bmp->pixels[2],pFrame_out->data[1],y_size/4); //U
                        memcpy(bmp->pixels[1],pFrame_out->data[2],y_size/4); //V
                        bmp->pitches[0]=pFrame_out->linesize[0];
                        bmp->pitches[2]=pFrame_out->linesize[1];
                        bmp->pitches[1]=pFrame_out->linesize[2];
                        SDL_UnlockYUVOverlay(bmp);
                        rect.x = 0;
                        rect.y = 0;
                        rect.w = pFrame_out->width;
                        rect.h = pFrame_out->height;
                        SDL_DisplayYUVOverlay(bmp, &rect);
                        //Delay 40ms
                        SDL_Delay(40);

                    }
                    av_frame_unref(pFrame_out);
                }
            }
            av_frame_unref(pFrame);
        }else if (packet.stream_index == audio_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_audio4(audioDecoderCtx, pFrame, &got_frame, &packet);
            if (ret < 0) {
                qInfo()<<( "Error decoding video\n");
                break;
            }

            if (got_frame) {

               decodeAndPlayAudioSample();
            }
            av_frame_unref(pFrame);
        }

        av_free_packet(&packet);
    }

end:
    avfilter_graph_free(&filter_graph);
    if (videoDecoderCtx)
        avcodec_close(videoDecoderCtx);
    avformat_close_input(&pFormatCtx);


    if (ret < 0 && ret != AVERROR_EOF) {
        char buf[1024];
        av_strerror(ret, buf, sizeof(buf));
        qInfo()<<"Error occurred: %s\n", buf;
        return -1;
    }

    return 0;
}

bool watermark::encodeWriteFrame(AVFrame *frame, int stream_index) {
    AVPacket encodedPacket;

   qDebug()<< "Encoding frame";
    encodedPacket.data = NULL;
    encodedPacket.size = 0;
    av_init_packet(&encodedPacket);
    int gotFrame = 0;
    if(pFormatCtx->streams[stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
        AVCodecContext *codec = outFormatCtx->streams[stream_index]->codec;
        if(avcodec_encode_video2(codec, &encodedPacket, frame, &gotFrame) <0){
            qDebug()<<"Failed to encode video";
            return false;
        }
    }

    //av_frame_free(&frame);

    if (!gotFrame){
        qDebug()<<"No got frame";
        return true;
    }


    /* prepare packet for muxing */
    encodedPacket.stream_index = stream_index;
    av_packet_rescale_ts(&encodedPacket,
                         outFormatCtx->streams[stream_index]->codec->time_base,
                         outFormatCtx->streams[stream_index]->time_base);


    writeFrameInOutput(encodedPacket);
}

bool watermark::writeFrameInOutput(AVPacket encodedPacket){
    qDebug()<<"Muxing frame";
     /* mux encoded frame */
     if(av_interleaved_write_frame(outFormatCtx, &encodedPacket) <0){
         qDebug() << "Failed to mux";
         return false;
     }
     return true;
}


bool watermark::openOutputFile(const char *filename){
    outFormatCtx = NULL;
    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, filename);
    if (!outFormatCtx) {
        qDebug() << "Could not create output context";
        return false;
    }

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            inVideoStream = pFormatCtx->streams[i];
            videoDecoderCtx = inVideoStream->codec;
            if(videoCodec == AV_CODEC_ID_NONE){
                videoCodec = AV_CODEC_ID_MPEG2VIDEO;
            }

            videoEncoder_ = avcodec_find_encoder(videoCodec);
            if(!videoEncoder_){
                qDebug()<< "Necessary encoder not found";
                return false;
            }

           videoEncoderCtx = avcodec_alloc_context3(videoEncoder_);
            if (!videoEncoderCtx) {
                qDebug()<< "Could not allocate video codec context";
                return false;
            }

            outVideoStream = avformat_new_stream(outFormatCtx, videoEncoder_);
            if(!outVideoStream){
                qDebug()<< "Failed allocating output stream";
                return false;
            }

            videoEncoderCtx->bit_rate = videoDecoderCtx->bit_rate;
            AVRational rational1 = {1,50};
            videoEncoderCtx->time_base = rational1;
            videoEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;

           videoEncoderCtx->sample_aspect_ratio = videoDecoderCtx->sample_aspect_ratio;
           if (videoEncoder_->pix_fmts)
               videoEncoderCtx->pix_fmt = videoEncoder_->pix_fmts[0];
           else
               videoEncoderCtx->pix_fmt = videoDecoderCtx->pix_fmt;
           videoEncoderCtx->height = videoDecoderCtx->height;
           videoEncoderCtx->width = videoDecoderCtx->width;
            if (avcodec_open2(videoEncoderCtx, videoEncoder_, NULL) < 0) {
                qDebug() << "Cannot open video encoder";
                return false;
            }


            if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
                videoEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            outFormatCtx->streams[i]->codec = videoEncoderCtx;
        }
    }

    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFormatCtx->pb, filename, AVIO_FLAG_WRITE) < 0) {
            qDebug()<< "Could not open output file " << filename;
            return false;
        }
    }

    /* init muxer, write output file header */
    if (avformat_write_header(outFormatCtx, NULL) < 0) {
        qDebug()<< "Error occurred when opening output file";
        return false;
    }

    av_dump_format(outFormatCtx, 0, filename, 1);

    return true;
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
