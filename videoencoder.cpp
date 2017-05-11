#include "videoencoder.h"


videoEncoder::videoEncoder(){
    initVars();
    initCodec();
}

void videoEncoder::initCodec(){
   av_register_all();
   avcodec_register_all();

   printf("License: %s\n",avformat_license());
   printf("AVCodec version %d\n", avformat_version());
   printf(avformat_configuration());
}

void videoEncoder::initVars(){
    outputFormat = NULL;
    inFormatCtx = NULL;
    outFormatCtx = NULL;
}

bool videoEncoder::remuxing(const char* inFilename, const char* outFilename){
    if (avformat_open_input(&inFormatCtx, inFilename, 0, 0) < 0) {
        qDebug() << "Could not open input file "<< inFilename;
        return false;
    }

    if (avformat_find_stream_info(inFormatCtx, 0) < 0) {
        qDebug()<< "Failed to retrieve input stream information";
        return false;
    }

    av_dump_format(inFormatCtx, 0, inFilename, 0);

    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outFilename);
    if (!outFormatCtx) {
        qDebug()<< "Could not create output context";
        return false;
    }

    outputFormat = outFormatCtx->oformat;

    for (int i = 0; i < (int)inFormatCtx->nb_streams; i++) {
        inStream = inFormatCtx->streams[i];
        outStream = avformat_new_stream(outFormatCtx, inStream->codec->codec);
        if (!outStream) {
            qDebug()<< "Failed allocating output stream";
            return false;
        }

        if (avcodec_copy_context(outStream->codec, inStream->codec) < 0) {
            qDebug()<< "Failed to copy context from input to output stream codec context";
            return false;
        }

        outStream->codec->codec_tag = 0;
        if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }


    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFormatCtx->pb, outFilename, AVIO_FLAG_WRITE) < 0) {
            qDebug()<< "Could not open output file "<< outFilename;
            return false;
        }
    }

    if (avformat_write_header(outFormatCtx, NULL) < 0) {
        qDebug()<<"Error occurred when opening output file";
        return false;
    }

    while (av_read_frame(inFormatCtx, &packet) >=0) {
        inStream  = inFormatCtx->streams[packet.stream_index];
        outStream = outFormatCtx->streams[packet.stream_index];


        AVRounding rr= AV_ROUND_NEAR_INF;
        //rr |= AV_ROUND_PASS_MINMAX;
        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, rr);
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base, rr);
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;

        if (av_interleaved_write_frame(outFormatCtx, &packet) < 0) {
            qDebug()<< "Error muxing packet";
            return false;
        }

        av_packet_unref(&packet);
    }

    av_dump_format(outFormatCtx, 0, outFilename, 1);

    av_write_trailer(outFormatCtx);

    avformat_close_input(&inFormatCtx);

    /* close output */
    if (outFormatCtx && !(outputFormat->flags & AVFMT_NOFILE))
        avio_closep(&outFormatCtx->pb);
    avformat_free_context(outFormatCtx);

    return true;
}

bool  videoEncoder::openInputFile(const char *filename){
    inFormatCtx = NULL;
    if (avformat_open_input(&inFormatCtx, filename, NULL, NULL) < 0) {
        qDebug()<< "Cannot open input file";
        return false;
    }

    if (avformat_find_stream_info(inFormatCtx, NULL) < 0) {
        qDebug()<<"Cannot find stream information";
        return false;
    }

    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        if(inFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoDecoderCtx = inFormatCtx->streams[i]->codec;
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
        else if(inFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audioDecoderCtx = inFormatCtx->streams[i]->codec;
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

    av_dump_format(inFormatCtx, 0, filename, 0);
    return true;
}

AVCodecID videoEncoder::getVideoCodecID(QString videoCodecStr){
    if(videoCodecStr == "MPEG1")
        return AV_CODEC_ID_MPEG1VIDEO;
    else if(videoCodecStr == "MPEG2")
        return AV_CODEC_ID_MPEG2VIDEO;
    else if(videoCodecStr == "")
        return AV_CODEC_ID_NONE;
}

AVCodecID videoEncoder::getAudioCodecID(QString audioCodecStr){
    if (audioCodecStr == "AAC")
        return AV_CODEC_ID_AAC;
    else if(audioCodecStr == "MP3")
        return AV_CODEC_ID_MP3;
    else if(audioCodecStr == "")
        return AV_CODEC_ID_NONE;
}

bool videoEncoder::openOutputFile(const char *filename, AVCodecID audioCodec, AVCodecID videoCodec){
    outFormatCtx = NULL;
    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, filename);
    if (!outFormatCtx) {
        qDebug() << "Could not create output context";
        return false;
    }

    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        if(inFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            inVideoStream = inFormatCtx->streams[i];
            videoDecoderCtx = inVideoStream->codec;
            if(videoCodec == AV_CODEC_ID_NONE){
                videoCodec = videoDecoderCtx->codec_id;
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

        }else if(inFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            outAudioStream = avformat_new_stream(outFormatCtx, NULL);
            if(!outAudioStream){
                qDebug()<< "Failed allocating output stream";
                return false;
            }
            inAudioStream = inFormatCtx->streams[i];
            audioDecoderCtx = inAudioStream->codec;
            audioEncoderCtx = outAudioStream->codec;
            if(audioCodec == AV_CODEC_ID_NONE){
                audioCodec = audioDecoderCtx->codec_id;
            }
            audioEncoder = avcodec_find_encoder(audioCodec);
            if(!audioEncoder){
                qDebug()<< "Necessary encoder not found";
                return false;
            }

            audioEncoderCtx->sample_rate = audioDecoderCtx->sample_rate;
            audioEncoderCtx->channel_layout = audioDecoderCtx->channel_layout;
            audioEncoderCtx->channels = av_get_channel_layout_nb_channels(audioEncoderCtx->channel_layout);
            audioEncoderCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; //audioEncoder->sample_fmts[0];
            AVRational rational = {1, audioEncoderCtx->sample_rate};
            audioEncoderCtx->time_base = rational;

            if (avcodec_open2(audioEncoderCtx, audioEncoder, NULL) < 0) {
                qDebug() << "Cannot open audio encoder";
                return false;
            }

            if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
                audioEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            outFormatCtx->streams[i]->codec = audioEncoderCtx;
        }

        else {
            // if this stream must be remuxed
            if (avcodec_copy_context(outFormatCtx->streams[i]->codec,inFormatCtx->streams[i]->codec) < 0) {
                qDebug()<< "Copying stream context failed";
                return false;
            }
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

bool videoEncoder::transcode(const char* in_filename, const char* out_filename, QString audioCodec, QString videoCodec){
    AVFrame *frame = NULL;
    int got_frame = 0;

    AVCodecID audioCodecID = getAudioCodecID(audioCodec);
    AVCodecID videoCodecID = getVideoCodecID(videoCodec);

    if (!openInputFile(in_filename))
        return false;
    if (!openOutputFile(out_filename, audioCodecID, videoCodecID))
        return false;

    /* read all packets */
    while ((av_read_frame(inFormatCtx, &packet)) >= 0) {
        qDebug()<< "Going to reencode the frame";
        frame = av_frame_alloc();
        if (!frame) {
            return false;
        }
        av_packet_rescale_ts(&packet,
                             inFormatCtx->streams[packet.stream_index]->time_base,
                             inFormatCtx->streams[packet.stream_index]->codec->time_base);

        if(inFormatCtx->streams[packet.stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            if(avcodec_decode_video2(inFormatCtx->streams[packet.stream_index]->codec, frame, &got_frame, &packet) <0){
                av_frame_free(&frame);
                qDebug()<< "Decoding failed";
                return false;
            }
        }else if(inFormatCtx->streams[packet.stream_index]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            if(avcodec_decode_audio4(inFormatCtx->streams[packet.stream_index]->codec, frame, &got_frame, &packet) <0){
                av_frame_free(&frame);
                qDebug()<< "Decoding failed";
                return false;
            }
        }

        if (got_frame) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            if(!encodeWriteFrame(frame, packet.stream_index, got_frame))
                return false;
         }
        av_packet_unref(&packet);
    }

    av_write_trailer(outFormatCtx);
    av_packet_unref(&packet);

    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        avcodec_close(inFormatCtx->streams[i]->codec);
        if (outFormatCtx && outFormatCtx->nb_streams > i && outFormatCtx->streams[i] && outFormatCtx->streams[i]->codec)
            avcodec_close(outFormatCtx->streams[i]->codec);
    }
    avformat_close_input(&inFormatCtx);
    if (outFormatCtx && !(outFormatCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outFormatCtx->pb);
    avformat_free_context(outFormatCtx);

    return true;
}

bool videoEncoder::encodeWriteFrame(AVFrame *frame, int stream_index, int gotFrame) {
    AVPacket encodedPacket;

   qDebug()<< "Encoding frame";
    encodedPacket.data = NULL;
    encodedPacket.size = 0;
    av_init_packet(&encodedPacket);

    if(inFormatCtx->streams[stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
        if(avcodec_encode_video2(outFormatCtx->streams[stream_index]->codec, &encodedPacket, frame, &gotFrame) <0){
            qDebug()<<"Failed to encode video";
            return false;
        }
    }else if (inFormatCtx->streams[stream_index]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
        if(avcodec_encode_audio2(outFormatCtx->streams[stream_index]->codec, &encodedPacket, frame, &gotFrame) <0){
            qDebug()<<"Failed to encode audio";
            return false;
        }
    }

    av_frame_free(&frame);

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

bool videoEncoder::writeFrameInOutput(AVPacket encodedPacket){
    qDebug()<<"Muxing frame";
     /* mux encoded frame */
     if(av_interleaved_write_frame(outFormatCtx, &encodedPacket) <0){
         qDebug() << "Failed to mux";
         return false;
     }
     return true;
}
