#include "videoremuxing.h"

videoRemuxing::videoRemuxing(){
    initVars();
    initCodec();
}

void videoRemuxing::initCodec(){
   av_register_all();
   avcodec_register_all();

   printf("License: %s\n",avformat_license());
   printf("AVCodec version %d\n", avformat_version());
   printf(avformat_configuration());
}

void videoRemuxing::initVars(){
    outputFormat = NULL;
    inFormatCtx = NULL;
    outFormatCtx = NULL;
}

bool videoRemuxing::remuxing(const char* inFilename, const char* outFilename){
    if (avformat_open_input(&inFormatCtx, inFilename, 0, 0) < 0) {
        printf("Could not open input file %s", inFilename);
        return false;
    }

    if (avformat_find_stream_info(inFormatCtx, 0) < 0) {
        printf("Failed to retrieve input stream information");
        return false;
    }

    av_dump_format(inFormatCtx, 0, inFilename, 0);

    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outFilename);
    if (!outFormatCtx) {
        printf("Could not create output context");
        return false;
    }

    outputFormat = outFormatCtx->oformat;

    for (int i = 0; i < (int)inFormatCtx->nb_streams; i++) {
        inStream = inFormatCtx->streams[i];
        outStream = avformat_new_stream(outFormatCtx, inStream->codec->codec);
        if (!outStream) {
            printf("Failed allocating output stream");
            return false;
        }

        if (avcodec_copy_context(outStream->codec, inStream->codec) < 0) {
            printf("Failed to copy context from input to output stream codec context");
            return false;
        }

        outStream->codec->codec_tag = 0;
        if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }


    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFormatCtx->pb, outFilename, AVIO_FLAG_WRITE) < 0) {
            printf("Could not open output file %s", outFilename);
            return false;
        }
    }

    if (avformat_write_header(outFormatCtx, NULL) < 0) {
        printf("Error occurred when opening output file");
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
            printf("Error muxing packet");
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
