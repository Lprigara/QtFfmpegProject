#include "videoencoder.h"

videoEncoder::videoEncoder(){
    initVars();
    initCodec();
}

void videoEncoder::initCodec(){
   avcodec_register_all();
   av_register_all();

   printf("License: %s\n",avformat_license());
   printf("AVCodec version %d\n", avformat_version());
}

void videoEncoder::initVars(){
    outputFormat = NULL;
    inFormatCtx = NULL;
    outFormatCtx = NULL;
}

void videoEncoder::remuxing(const char* in_filename, const char* out_filename){

    int ret;

    if ((ret = avformat_open_input(&inFormatCtx, in_filename, 0, 0)) < 0) {
        qDebug() << "Could not open input file "<< in_filename;
        return;
    }

    if ((ret = avformat_find_stream_info(inFormatCtx, 0)) < 0) {
        qDebug()<< "Failed to retrieve input stream information";
        return;
    }

    av_dump_format(inFormatCtx, 0, in_filename, 0);

    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, out_filename);
    if (!outFormatCtx) {
        qDebug()<< "Could not create output context";
        return;
    }

    outputFormat = outFormatCtx->oformat;

    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        inStream = inFormatCtx->streams[i];
        outStream = avformat_new_stream(outFormatCtx, inStream->codec->codec);
        if (!outStream) {
            qDebug()<< "Failed allocating output stream";
            return;
        }

        ret = avcodec_copy_context(outStream->codec, inStream->codec);
        if (ret < 0) {
            qDebug()<< "Failed to copy context from input to output stream codec context";
            return;
        }
        outStream->codec->codec_tag = 0;
        if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    av_dump_format(outFormatCtx, 0, out_filename, 1);

    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFormatCtx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            qDebug()<< "Could not open output file "<< out_filename;
            return;
        }
    }

    ret = avformat_write_header(outFormatCtx, NULL);
    if (ret < 0) {
        qDebug()<<"Error occurred when opening output file";
        return;
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

        ret = av_interleaved_write_frame(outFormatCtx, &packet);
        if (ret < 0) {
            qDebug()<< "Error muxing packet";
            return;
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(outFormatCtx);
}
