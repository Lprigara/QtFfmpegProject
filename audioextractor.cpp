#include "audioextractor.h"

audioExtractor::audioExtractor(){
    avcodec_register_all();
    av_register_all();
    avfilter_register_all();

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

bool audioExtractor::init_filter(){
    buffersinkCtx = NULL;
    buffersrcCtx = NULL;
    filterGraph = NULL;

    char args[512];
    AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    const enum AVSampleFormat out_sample_fmts[] = { outAudioCodecCtx->sample_fmt, AVSampleFormat::AV_SAMPLE_FMT_NONE };
    const int64_t out_channel_layouts[] = { outAudioCodecCtx->channel_layout, -1 };
    const int out_sample_rates[] = { outAudioCodecCtx->sample_rate, -1 };
    const AVFilterLink *inlink;
    const AVFilterLink *outlink;
    AVRational time_base = informatCtx->streams[audioStreamIndex]->time_base;
    filterGraph = avfilter_graph_alloc();

    if (!inAudioCodecCtx->channel_layout) {
        inAudioCodecCtx->channel_layout = av_get_default_channel_layout(inAudioCodecCtx->channels);
    }

    snprintf(args, sizeof(args),
        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
          time_base.num, time_base.den, inAudioCodecCtx->sample_rate,
          av_get_sample_fmt_name(inAudioCodecCtx->sample_fmt), inAudioCodecCtx->channel_layout);

    if(avfilter_graph_create_filter(&buffersrcCtx, abuffersrc, "in", args, NULL, filterGraph) < 0) {
        qDebug() << "Cannot create audio buffer source";
        return false;
    }

    // set up buffer audio sink
    avfilter_graph_create_filter(&buffersinkCtx, abuffersink, "out", NULL, NULL, filterGraph);
    av_opt_set_int_list(buffersinkCtx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int_list(buffersinkCtx, "channel_layouts", out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int_list(buffersinkCtx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);

    // Endpoints for the filter graph
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrcCtx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersinkCtx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    snprintf(args, sizeof(args),
        "aresample=%d,aformat=sample_fmts=%s:channel_layouts=stereo",
        outAudioCodecCtx->sample_rate,
        av_get_sample_fmt_name(outAudioCodecCtx->sample_fmt));

    if ((avfilter_graph_parse(filterGraph, args, inputs, outputs, NULL)) < 0)
        return false;

    if ((avfilter_graph_config(filterGraph, NULL)) < 0)
        return false;

    // Make sure that the output frames have the correct size
    if (outAudioCodec->type == AVMEDIA_TYPE_AUDIO && !(outAudioCodec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)) {
        av_buffersink_set_frame_size(buffersinkCtx, outAudioCodecCtx->frame_size);
    }

    // Print summary of the source buffer
    inlink = buffersrcCtx->outputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, inlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Input: sample rate: %dHz; samlpe format:%s; channel layout:%s\n",
          (int)inlink->sample_rate,
          (char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat) inlink->format), "?"),
          args);

    // Print summary of the sink buffer
    outlink = buffersinkCtx->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Output: sample rate: %dHz; samlpe format:%s; channel layout:%s\n",
          (int)outlink->sample_rate,
          (char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat) outlink->format), "?"),
          args);

    return true;
}

bool audioExtractor::transcode(){
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filteredFrame = av_frame_alloc();
    int gotFrame = 0;
    int ret = 0;
    av_init_packet(&packet);

    if(!init_filter()) {
        qDebug() << "Could not initialize audio filter.";
        return false;
    }

    while(av_read_frame(informatCtx, &packet) >= 0){
        if(packet.stream_index == audioStreamIndex){
            avcodec_get_frame_defaults(frame);
            gotFrame = 0;
            if(avcodec_decode_audio4(inAudioCodecCtx, frame, &gotFrame, &packet) < 0) {
                qDebug() << "Error decoding audio frame.";
                continue;
            }

            if(gotFrame){
                // push frame into filter
                if(av_buffersrc_add_frame_flags(buffersrcCtx, frame, AV_BUFFERSRC_FLAG_PUSH) < 0){
                    qDebug() << "Error while feeding the audio filtergraph";
                }

                while(1) {
                    // pull filtered frames
                    ret = av_buffersink_get_frame(buffersinkCtx, filteredFrame);
                    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if(ret < 0){
                        // write frame that are left in the queue
                        flush_queue(outAudioCodecCtx);

                        av_frame_free(&frame);
                        av_write_trailer(outformatCtx);

                        return false;
                    }

                    filteredFrame->pts = AV_NOPTS_VALUE;

                    // Write the decoded and converted audio frame
                    if(!write_audio_frame(filteredFrame)) {
                      return false;
                    }

                    av_frame_unref(filteredFrame);
                }
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
