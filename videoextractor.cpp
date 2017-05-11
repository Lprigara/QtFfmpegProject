#include "videoextractor.h"

videoExtractor::videoExtractor(){
    avcodec_register_all();
    av_register_all();

    inVideoCodec = NULL;
    inVideoCodecCtx = NULL;
    videoStreamIndex = 0;
    informatCtx = NULL;
    outVideoCodec = NULL;
    outVideoCodecCtx = NULL;
    outformatCtx = NULL;
    outputFormat = NULL;
    videoStream = NULL;
}

bool videoExtractor::convert(const char* inputFileName, const char* outputFileName){
    if(!openInputFile(inputFileName))
        return false;

    if(!open_output_file(outputFileName))
        return false;

    if(transcode() < 0)
        return false;

    avcodec_close(inVideoCodecCtx);
    avcodec_close(outVideoCodecCtx);

    avformat_close_input(&informatCtx);
    avio_close(outformatCtx->pb);

    return true;
}

bool videoExtractor::openInputFile(const char* inputFileName){
    // Open input file
    if(avformat_open_input(&informatCtx, inputFileName, NULL, NULL) < 0)
        return false; // Couldn't open file

     if(avformat_find_stream_info(informatCtx, NULL) < 0)
        return false;

    videoStreamIndex = av_find_best_stream(informatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &inVideoCodec, 0);
    if(videoStreamIndex < 0)
        return false;  // Could not  find video stream

    inVideoCodecCtx = informatCtx->streams[videoStreamIndex]->codec;

    // Init the video codec
    if(avcodec_open2(inVideoCodecCtx, inVideoCodec, NULL) < 0)
        return false;

    return true;
}

bool videoExtractor::open_output_file(const char* outputFileName){
    avformat_alloc_output_context2(&outformatCtx, NULL, NULL, outputFileName);
    outputFormat = outformatCtx->oformat;

    if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
        // Add video stream to output format
        videoStream = add_stream(&outVideoCodec, outputFormat->video_codec);
        if(videoStream == NULL)
            return false;

        outVideoCodecCtx = videoStream->codec;
    }

    // open video codec
    if (avcodec_open2(outVideoCodecCtx, outVideoCodec, NULL)) {
        qDebug() << "Could not open video codec: " << outVideoCodec->name;
        return false;
    }

    av_dump_format(outformatCtx, 0, outputFileName, 1);

    /* open the output file, if needed */
    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        if(avio_open(&outformatCtx->pb, outputFileName, AVIO_FLAG_WRITE) < 0) {
            qDebug() << "Could not open " << outputFileName;
            return false;
        }
    }

    /* Write the stream header, if any. */
    if(avformat_write_header(outformatCtx, NULL) < 0) {
        qDebug() << "Error occurred when opening output file";
        return false;
    }

    return true;
}

bool videoExtractor::transcode(){
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    int gotFrame = 0;
    av_init_packet(&packet);

    while(av_read_frame(informatCtx, &packet) >= 0){
        if(packet.stream_index == videoStreamIndex){
            av_frame_unref(frame);
            gotFrame = 0;
            if(avcodec_decode_video2(inVideoCodecCtx, frame, &gotFrame, &packet) < 0) {
                qDebug() << "Error decoding video frame.";
                continue;
            }

            if(gotFrame){
                frame->pts = AV_NOPTS_VALUE;

                // Write the decoded and converted video frame
                if(!writeVideoFrame(frame)) {
                  return false;
                }

                av_frame_unref(frame);
            }
        }

        av_free_packet(&packet);
    }
    return true;
}

AVStream* videoExtractor::add_stream(AVCodec **codec, enum AVCodecID codec_id){
  AVCodecContext *codecCtx;
  AVStream *stream;

  *codec = avcodec_find_encoder(codec_id);

  if (!(*codec)) {
      qDebug() << "Could not find encoder for " << avcodec_get_name(codec_id);
      return NULL;
  }

  stream = avformat_new_stream(outformatCtx, *codec);
  if (!stream) {
      qDebug() << "Could not allocate stream";
      return NULL;
  }

  //stream->id = 0;
  codecCtx = stream->codec;
  codecCtx->bit_rate = inVideoCodecCtx->bit_rate;
  codecCtx->sample_fmt = inVideoCodecCtx->sample_fmt;
  codecCtx->pix_fmt = inVideoCodecCtx->pix_fmt;
  codecCtx->channels = inVideoCodecCtx->channels;
  codecCtx->time_base = inVideoCodecCtx->time_base;
  codecCtx->height = inVideoCodecCtx->height;
  codecCtx->width = inVideoCodecCtx->width;

  return stream;
}

bool videoExtractor::writeVideoFrame(AVFrame *frame){
  AVCodecContext *codec = outformatCtx->streams[0]->codec;
  AVPacket packet;
  int gotPacket = 0;

  av_init_packet(&packet);
  packet.data = NULL;
  packet.size = 0;

  if(avcodec_encode_video2(codec, &packet, frame, &gotPacket) < 0){
      qDebug() << "Error encoding video frame.";
      return false;
  }

  if (gotPacket){

        packet.pts = av_rescale_q(packet.pts, codec->time_base, outformatCtx->streams[0]->time_base);
        packet.dts = av_rescale_q(packet.dts, codec->time_base, outformatCtx->streams[0]->time_base);
    if (packet.duration > 0)
        packet.duration = (int) av_rescale_q(packet.duration, codec->time_base, outformatCtx->streams[0]->time_base);

    /* Write the compressed frame to the media file. */
    if(av_interleaved_write_frame(outformatCtx, &packet) != 0) {
        qDebug() << "Error while writing video frame: ";
        return false;
    }

    av_free_packet(&packet);
  }

  return true;
}

void videoExtractor::flush_queue(AVCodecContext *codec){
  AVPacket packet;
  av_init_packet(&packet);
  packet.data = NULL;
  packet.size = 0;
  int gotPacket = 0;

  for(gotPacket = 1;gotPacket;) {
    if(avcodec_encode_video2(codec, &packet, NULL, &gotPacket) < 0 || !gotPacket)
      return;

    av_interleaved_write_frame (outformatCtx, &packet);
    av_free_packet(&packet);
  }

  av_interleaved_write_frame (outformatCtx, NULL);
}

void videoExtractor::avcodec_get_frame_defaults(AVFrame *frame){
     // extended_data should explicitly be freed when needed, this code is unsafe currently
     // also this is not compatible to the <55 ABI/API
    if (frame->extended_data != frame->data && 0)
        av_freep(&frame->extended_data);

    memset(frame, 0, sizeof(AVFrame));
    av_frame_unref(frame);
}

