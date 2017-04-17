#include "audioextractor.h"

audioExtractor::audioExtractor(){
    avcodec_register_all();
    av_register_all();

    inAudioCodec = NULL;
    inAudioCodecCtx = NULL;
    audioStreamIndex = 0;
    informatCtx = NULL;
    outAudioCodec = NULL;
    outAudioCodecCtx = NULL;
    outformatCtx = NULL;
    outputFormat = NULL;
    audioStream = NULL;
}

void audioExtractor::set_output_sample_fmt(){
  if (outAudioCodec && outAudioCodec->sample_fmts) {

    // check if the output encoder supports the input sample format
    const enum AVSampleFormat *sampleFormat = outAudioCodec->sample_fmts;
    for (; *sampleFormat != -1; sampleFormat++) {
      if (*sampleFormat == inAudioCodecCtx->sample_fmt) {
        outAudioCodecCtx->sample_fmt = *sampleFormat;
        break;
      }
    }

    if (*sampleFormat == -1) {
      // if not, we need to convert sample formats and select the first format that is supported by the output encoder
      outAudioCodecCtx->sample_fmt = outAudioCodec->sample_fmts[0];
    }
  }
}

AVStream* audioExtractor::add_stream(AVCodec **codec, enum AVCodecID codec_id){
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

  stream->id = 1;
  codecCtx = stream->codec;
  codecCtx->bit_rate = inAudioCodecCtx->bit_rate;
  codecCtx->sample_rate = inAudioCodecCtx->sample_rate;
  codecCtx->channels = inAudioCodecCtx->channels;
  codecCtx->channel_layout = inAudioCodecCtx->channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

  return stream;
}

bool audioExtractor::write_audio_frame(AVFrame *frame){
  AVCodecContext *codec = outformatCtx->streams[0]->codec;
  AVPacket packet;
  int gotPacket = 0;

  av_init_packet(&packet);
  packet.data = NULL;
  packet.size = 0;

  if(avcodec_encode_audio2(codec, &packet, frame, &gotPacket) < 0){
      qDebug() << "Error encoding audio frame.";
      return false;
  }

  if (gotPacket){
    if (packet.pts != AV_NOPTS_VALUE)
        packet.pts = av_rescale_q(packet.pts, codec->time_base, outformatCtx->streams[0]->time_base);
    if (packet.dts != AV_NOPTS_VALUE)
        packet.dts = av_rescale_q(packet.dts, codec->time_base, outformatCtx->streams[0]->time_base);
    if (packet.duration > 0)
        packet.duration = (int) av_rescale_q(packet.duration, codec->time_base, outformatCtx->streams[0]->time_base);

    /* Write the compressed frame to the media file. */
    if(av_interleaved_write_frame(outformatCtx, &packet) != 0) {
        qDebug() << "Error while writing audio frame: ";
        return false;
    }

    av_free_packet(&packet);
  }

  return true;
}

void audioExtractor::flush_queue(AVCodecContext *codec){
  AVPacket packet;
  av_init_packet(&packet);
  packet.data = NULL;
  packet.size = 0;
  int gotPacket = 0;

  for(gotPacket = 1;gotPacket;) {
    if(avcodec_encode_audio2(codec, &packet, NULL, &gotPacket) < 0 || !gotPacket)
      return;

    av_interleaved_write_frame (outformatCtx, &packet);
    av_free_packet(&packet);
  }

  av_interleaved_write_frame (outformatCtx, NULL);
}

bool audioExtractor::transcode(){
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    int gotFrame = 0;
    av_init_packet(&packet);

    while(av_read_frame(informatCtx, &packet) >= 0){
        if(packet.stream_index == audioStreamIndex){
            avcodec_get_frame_defaults(frame);
            gotFrame = 0;
            if(avcodec_decode_audio4(inAudioCodecCtx, frame, &gotFrame, &packet) < 0) {
                qDebug() << "Error decoding audio frame.";
                continue;
            }

            if(gotFrame){
                frame->pts = AV_NOPTS_VALUE;

                // Write the decoded and converted audio frame
                if(!write_audio_frame(frame)) {
                  return false;
                }

                av_frame_unref(frame);
            }
        }

        av_free_packet(&packet);
    }
    return true;
}

bool audioExtractor::open_input_file(const char* inputFileName){
  // Open input file
  if(avformat_open_input(&informatCtx, inputFileName, NULL, NULL) < 0)
    return false; // Couldn't open file

   if(avformat_find_stream_info(informatCtx, NULL) < 0)
    return false;

  audioStreamIndex = av_find_best_stream(informatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &inAudioCodec, 0);
  if(audioStreamIndex < 0)
    return false;  // Could not  find audio stream

  inAudioCodecCtx = informatCtx->streams[audioStreamIndex]->codec;

    // Init the audio codec
  if(avcodec_open2(inAudioCodecCtx, inAudioCodec, NULL) < 0)
    return false;

  return true;
}

bool audioExtractor::open_output_file(const char* outputFileName){
    avformat_alloc_output_context2(&outformatCtx, NULL, NULL, outputFileName);
    outputFormat = outformatCtx->oformat;

    if (outputFormat->audio_codec != AV_CODEC_ID_NONE) {
        // Add audio stream to output format
        audioStream = add_stream(&outAudioCodec, outputFormat->audio_codec);
        if(audioStream == NULL)
            return false;

        outAudioCodecCtx = audioStream->codec;
    }

    set_output_sample_fmt();

    // open audio codec
    if (avcodec_open2(outAudioCodecCtx, outAudioCodec, NULL)) {
        qDebug() << "Could not open audio codec: " << outAudioCodec->name;
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

bool audioExtractor::convert(const char* inputFileName, const char* outputFileName){
  if(!open_input_file(inputFileName))
    return false;

  if(!open_output_file(outputFileName))
    return false;

  if(transcode() < 0)
    return false;

  avcodec_close(inAudioCodecCtx);
  avcodec_close(outAudioCodecCtx);

  avformat_close_input(&informatCtx);
  avio_close(outformatCtx->pb);

  return true;
}

void audioExtractor::avcodec_get_frame_defaults(AVFrame *frame){
     // extended_data should explicitly be freed when needed, this code is unsafe currently
     // also this is not compatible to the <55 ABI/API
    if (frame->extended_data != frame->data && 0)
        av_freep(&frame->extended_data);

    memset(frame, 0, sizeof(AVFrame));
    av_frame_unref(frame);
}
