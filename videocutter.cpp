#include "videocutter.h"

videoCutter::videoCutter(){
    av_register_all();

    outputFormat = NULL;
    inFormatCtx = NULL;
    outFormatCtx = NULL;
}

bool videoCutter::cut(double startTime, double endTime, const char* inFilename, const char* outFilename) {

    //Open the input file and allocate the information in container
    if (avformat_open_input(&inFormatCtx, inFilename, 0, 0) < 0) {
        printf("Could not open input file %s", inFilename);
        return false;
    }

    //Retrieve streams information
    if (avformat_find_stream_info(inFormatCtx, 0) < 0) {
        printf("Failed to retrieve input stream information");
        return false;
    }

    //Dump information in screen
    av_dump_format(inFormatCtx, 0, inFilename, 0);

    //Create an output container with the information relative to outfile extension
    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outFilename);
    if (!outFormatCtx) {
        printf("Could not create output context");
        return false;
    }

    outputFormat = outFormatCtx->oformat;

    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        inStream = inFormatCtx->streams[i];
        //Create new stream in output container
        outStream = avformat_new_stream(outFormatCtx, inStream->codec->codec);
        if (!outStream) {
            printf("Failed allocating output stream");
            return false;
        }

        //Copy input codec information in output codec
        if(avcodec_copy_context(outStream->codec, inStream->codec) < 0) {
            printf("Failed to copy context from input to output stream codec context");
            return false;
        }

        outStream->codec->codec_tag = 0;
        if (outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
            outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //Dump info in screen
    av_dump_format(outFormatCtx, 0, outFilename, 1);

    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        if(avio_open(&outFormatCtx->pb, outFilename, AVIO_FLAG_WRITE) <0) {
            printf("Could not open output file '%s'", outFilename);
            return false;
        }
    }

    //Open ouput file to write in it
    if(avformat_write_header(outFormatCtx, NULL) < 0) {
        printf("Error occurred when opening output file");
        return false;
    }

    //Go to frame seek
    if(av_seek_frame(inFormatCtx, -1, startTime*AV_TIME_BASE, AVSEEK_FLAG_ANY) < 0) {
        printf("Error seek");
        return false;
    }

    int64_t *dts_start_from = (int64_t*)malloc(sizeof(int64_t) * inFormatCtx->nb_streams);
    memset(dts_start_from, 0, sizeof(int64_t) * inFormatCtx->nb_streams);
    int64_t *pts_start_from = (int64_t*)malloc(sizeof(int64_t) * inFormatCtx->nb_streams);
    memset(pts_start_from, 0, sizeof(int64_t) * inFormatCtx->nb_streams);

    //iterate over the frames
    while (av_read_frame(inFormatCtx, &packet) >= 0) {
        inStream  = inFormatCtx->streams[packet.stream_index];
        outStream = outFormatCtx->streams[packet.stream_index];

        if (av_q2d(inStream->time_base) * packet.pts > endTime) {
            av_free_packet(&packet);
            break;
        }

        if (dts_start_from[packet.stream_index] == 0) {
            dts_start_from[packet.stream_index] = packet.dts;
        }
        if (pts_start_from[packet.stream_index] == 0) {
            pts_start_from[packet.stream_index] = packet.pts;
        }

        /* copy packet */
        packet.pts = av_rescale_q_rnd(packet.pts - pts_start_from[packet.stream_index], inStream->time_base, outStream->time_base, AV_ROUND_NEAR_INF/*|AV_ROUND_PASS_MINMAX*/);
        packet.dts = av_rescale_q_rnd(packet.dts - dts_start_from[packet.stream_index], inStream->time_base, outStream->time_base, AV_ROUND_NEAR_INF/*|AV_ROUND_PASS_MINMAX*/);
        if (packet.pts < 0) {
            packet.pts = 0;
        }
        if (packet.dts < 0) {
            packet.dts = 0;
        }
        packet.duration = (int)av_rescale_q((int64_t)packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;

        if(av_interleaved_write_frame(outFormatCtx, &packet) < 0) {
            printf("Error muxing packet");
            return false;
        }
        av_free_packet(&packet);
    }

    free(dts_start_from);
    free(pts_start_from);

    av_write_trailer(outFormatCtx);

    avformat_close_input(&inFormatCtx);

    /* close output */
    if (outFormatCtx && !(outputFormat->flags & AVFMT_NOFILE))
        avio_closep(&outFormatCtx->pb);
    avformat_free_context(outFormatCtx);

    return true;
}
