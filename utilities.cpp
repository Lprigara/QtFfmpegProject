#include "utilities.h"

utilities::utilities(){}

void utilities::dumpFormat(QFile* file, AVFormatContext *formatCtx, int index, int is_output){

    QTextStream outFile(file);

    uint8_t *printed = formatCtx->nb_streams ?  (uint8_t*)av_mallocz(formatCtx->nb_streams) : NULL;
    if (formatCtx->nb_streams && !printed)
        return;

    if(is_output)
        outFile << "Output #" << index << ", " << formatCtx->oformat->name << " to '" << file->fileName() << "' \n";
    else
        outFile << "Input #" << index << ", " << formatCtx->iformat->name << " from '" << file->fileName() << "' \n";

    dumpMetadata(outFile, formatCtx->metadata);


    if (!is_output) {
        outFile << "Duration: ";
        if (formatCtx->duration != AV_NOPTS_VALUE){
            int hours, mins, secs, us;
            secs = formatCtx->duration / AV_TIME_BASE;
            us = formatCtx->duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            outFile << " " << hours <<":" << mins <<":" << secs <<"." <<(100 * us) / AV_TIME_BASE;
        }else {
            outFile <<"N/A \n";
        }

        if (formatCtx->start_time != AV_NOPTS_VALUE){
            int secs, us;
            outFile <<", start: ";
            secs = formatCtx->start_time / AV_TIME_BASE;
            us = formatCtx->start_time % AV_TIME_BASE;
            outFile << secs << "." << (int)av_rescale(us, 1000000, AV_TIME_BASE);
        }

        outFile <<", bitrate: ";
        if (formatCtx->bit_rate) {
            outFile << formatCtx->bit_rate / 1000 << " kb/s\n" ;
        } else {
            outFile << "N/A\n";
        }
        outFile <<"\n";
    }


    if(formatCtx->nb_programs) {
        int total=0;
        for(int j=0; j<formatCtx->nb_programs; j++) {
            AVDictionaryEntry *name = av_dict_get(formatCtx->programs[j]->metadata, "name", NULL, 0);
            if(name)
                outFile <<"  Program "<< formatCtx->programs[j]->id << " " << name->value << "\n";
            else
                outFile <<"  Program "<< formatCtx->programs[j]->id << "  \n";

            for(int k=0; k<formatCtx->programs[j]->nb_stream_indexes; k++) {
                dumpStreamFormat(outFile, formatCtx, formatCtx->programs[j]->stream_index[k], index, is_output);
                printed[formatCtx->programs[j]->stream_index[k]] = 1;
            }
            total += formatCtx->programs[j]->nb_stream_indexes;
        }
        if (total < formatCtx->nb_streams)
            outFile << "  No Program\n";
    }

    for (int i = 0; i < formatCtx->nb_streams; i++)
        if (!printed[i])
            dumpStreamFormat(outFile, formatCtx, i, index, is_output);

    av_free(printed);
}

void utilities::dumpMetadata(QTextStream& outFile, AVDictionary *metadata){
    if (metadata && !(av_dict_count(metadata) == 1 && av_dict_get(metadata, "language", NULL, 0))) {
        AVDictionaryEntry *tag = NULL;

        outFile << "   Metadata:\n";

        while ((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX))){
            if (strcmp("language", tag->key)) {
                const char *tagValue = tag->value;
                outFile << "   " << tag->key;
                while (*tagValue) {
                    char tmp[256];
                    size_t len = strcspn(tagValue, "\x8\xa\xb\xc\xd");
                    av_strlcpy(tmp, tagValue, FFMIN(sizeof(tmp), len+1));
                    outFile << tmp;
                    tagValue += len;
                    if (*tagValue == 0xd) outFile << " ";
                    if (*tagValue == 0xa) outFile << " \n";
                    if (*tagValue) tagValue++;
                }
                outFile << "\n";
            }
        }
    }
}

void utilities::dumpStreamFormat(QTextStream& outFile, AVFormatContext* formatCtx, int i, int index, int is_output){
    char buf[256];
    int flags = (is_output ? formatCtx->oformat->flags : formatCtx->iformat->flags);
    AVStream *stream = formatCtx->streams[i];
    AVDictionaryEntry *lang = av_dict_get(stream->metadata, "language", NULL, 0);
    char *separator = (char*) formatCtx->dump_separator;
    AVCodecContext *codecCtx;
    int ret;

    codecCtx = avcodec_alloc_context3(NULL);
    if (!codecCtx)
        return;

    ret = avcodec_parameters_to_context(codecCtx, stream->codecpar);
    if (ret < 0) {
        avcodec_free_context(&codecCtx);
        return;
    }

    // Fields which are missing from AVCodecParameters need to be taken from the AVCodecContext
    codecCtx->properties = stream->codec->properties;
    codecCtx->codec      = stream->codec->codec;
    codecCtx->qmin       = stream->codec->qmin;
    codecCtx->qmax       = stream->codec->qmax;
    codecCtx->coded_width  = stream->codec->coded_width;
    codecCtx->coded_height = stream->codec->coded_height;

    if (separator)
        av_opt_set(codecCtx, "dump_separator", separator, 0);
    avcodec_string(buf, sizeof(buf), codecCtx, is_output);
    avcodec_free_context(&codecCtx);

    outFile <<"Stream #" <<index << ":" <<i;

    /* the pid is an important information, so we display it */
    if (flags & AVFMT_SHOW_IDS)
        outFile << "[0x" << stream->id <<"]";
    if (lang)
        outFile <<"(" << lang->value <<")";

    outFile << ", "<< stream->codec_info_nb_frames << ", " << stream->time_base.num << "/" << stream->time_base.den;
    outFile << ": " << buf;

    if (stream->sample_aspect_ratio.num && av_cmp_q(stream->sample_aspect_ratio, stream->codecpar->sample_aspect_ratio)) {
        AVRational display_aspect_ratio;
        av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  stream->codecpar->width  * (int64_t)stream->sample_aspect_ratio.num,
                  stream->codecpar->height * (int64_t)stream->sample_aspect_ratio.den,
                  1024 * 1024);
        outFile <<", SAR " << stream->sample_aspect_ratio.num << ":" << stream->sample_aspect_ratio.den << ", DAR " <<
                  display_aspect_ratio.num << ":" << display_aspect_ratio.den;
    }

    if (stream->disposition & AV_DISPOSITION_DEFAULT)
        outFile << " (default)";
    if (stream->disposition & AV_DISPOSITION_DUB)
        outFile << " (dub)";
    if (stream->disposition & AV_DISPOSITION_ORIGINAL)
        outFile << " (original)";
    if (stream->disposition & AV_DISPOSITION_COMMENT)
        outFile << " (comment)";
    if (stream->disposition & AV_DISPOSITION_LYRICS)
        outFile << " (lyrics)";
    if (stream->disposition & AV_DISPOSITION_KARAOKE)
        outFile << " (karaoke)";
    if (stream->disposition & AV_DISPOSITION_FORCED)
        outFile << " (forced)";
    if (stream->disposition & AV_DISPOSITION_HEARING_IMPAIRED)
        outFile << " (hearing impaired)";
    if (stream->disposition & AV_DISPOSITION_VISUAL_IMPAIRED)
        outFile << " (visual impaired)";
    if (stream->disposition & AV_DISPOSITION_CLEAN_EFFECTS)
        outFile << " (clean effects)";
    outFile << "\n";

    dumpMetadata(outFile, stream->metadata);

    dumpSidedata(outFile, stream);
}

double av_display_rotation_get(const int32_t matrix[9]){

    // fixed point to double
    #define CONV_FP(x) ((double) (x)) / (1 << 16)

    // double to fixed point
    #define CONV_DB(x) (int32_t) ((x) * (1 << 16))

    double rotation, scale[2];

    scale[0] = hypot(CONV_FP(matrix[0]), CONV_FP(matrix[3]));
    scale[1] = hypot(CONV_FP(matrix[1]), CONV_FP(matrix[4]));

    if (scale[0] == 0.0 || scale[1] == 0.0)
        return NAN;

    rotation = atan2(CONV_FP(matrix[1]) / scale[1],
                     CONV_FP(matrix[0]) / scale[0]) * 180 / M_PI;

    return -rotation;
}

void utilities::dumpSidedata(QTextStream& outFile, AVStream *stream){

    if (stream->nb_side_data)
        outFile <<"   Side data:\n";

    for (int i = 0; i < stream->nb_side_data; i++) {
        AVPacketSideData sideData = stream->side_data[i];
        outFile << "         ";

        switch (sideData.type) {
        case AV_PKT_DATA_PALETTE:
            outFile << "palette";
            break;
        case AV_PKT_DATA_NEW_EXTRADATA:
            outFile << "new extradata";
            break;
        case AV_PKT_DATA_PARAM_CHANGE:
            outFile << "paramchange: ";
            dumpParamChange(outFile, &sideData);
            break;
        case AV_PKT_DATA_H263_MB_INFO:
            outFile << "H.263 macroblock info";
            break;
        case AV_PKT_DATA_REPLAYGAIN:
            outFile << "replaygain: ";
            dumpReplaygain(outFile, &sideData);
            break;
        case AV_PKT_DATA_DISPLAYMATRIX:
            outFile << "displaymatrix: rotation of " << av_display_rotation_get((int32_t *)sideData.data) << " degrees";
            break;
        case AV_PKT_DATA_STEREO3D:
            outFile << "stereo3d: ";
            dumpStereo3d(outFile, &sideData);
            break;
        case AV_PKT_DATA_AUDIO_SERVICE_TYPE:
            outFile << "audio service type: ";
            dumpAudioServiceType(outFile, &sideData);
            break;
        case AV_PKT_DATA_QUALITY_STATS:
            outFile << "quality factor: " << AV_RL32(sideData.data) << ", pict_type: " << av_get_picture_type_char((AVPictureType)sideData.data[4]);
            break;
        case AV_PKT_DATA_CPB_PROPERTIES:
            outFile << "cpb: ";
            dumpCPB(outFile, &sideData);
            break;
        default:
            outFile << "unknown side data type " <<  sideData.type << "(" << sideData.size << " bytes)";
            break;
        }

        outFile << "\n";
    }
}

void utilities::dumpParamChange(QTextStream& outFile, AVPacketSideData *sideData)
{
    int size = sideData->size;
    const uint8_t *data = sideData->data;
    uint32_t flags, channels, sample_rate, width, height;
    uint64_t layout;

    if (!data || sideData->size < 4){
         outFile <<"unknown param";
    }

    flags = AV_RL32(data);
    data += 4;
    size -= 4;

    if (flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT) {
        if (size < 4){
            outFile <<"unknown param";
            return;
        }
        channels = AV_RL32(data);
        data += 4;
        size -= 4;
        outFile << "channel count: "<< channels;
    }
    if (flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT) {
        if (size < 8){
            outFile <<"unknown param";
            return;
        }
        layout = AV_RL64(data);
        data += 8;
        size -= 8;
        outFile << "channel layout: " << av_get_channel_name(layout);
    }
    if (flags & AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE) {
        if (size < 4){
            outFile <<"unknown param";
            return;
        }
        sample_rate = AV_RL32(data);
        data += 4;
        size -= 4;
        outFile << "sample_rate: " << sample_rate;
    }
    if (flags & AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS) {
        if (size < 8){
            outFile <<"unknown param";
            return;
        }
        width = AV_RL32(data);
        data += 4;
        size -= 4;
        height = AV_RL32(data);
        data += 4;
        size -= 4;
        outFile << "width "<< width << ", height " << height;
    }
}

static void print_gain(QTextStream &outFile, const char *str, int32_t gain)
{
    outFile << str << " - ";
    if (gain == INT32_MIN)
        outFile << "unknown";
    else
        outFile << gain / 100000.0f;
    outFile << ", ";
}

static void print_peak(QTextStream &outFile, const char *str, uint32_t peak)
{
    outFile << str << " - ";
    if (!peak)
        outFile << "unknown";
    else
        outFile << (float) peak / UINT32_MAX;
    outFile << ", ";
}

void utilities::dumpReplaygain(QTextStream &outFile, AVPacketSideData *sideData)
{
    AVReplayGain *replayGain;

    if (sideData->size < sizeof(*replayGain)) {
        outFile << "invalid data";
        return;
    }
    replayGain = (AVReplayGain*)sideData->data;

    print_gain(outFile, "track gain", replayGain->track_gain);
    print_peak(outFile, "track peak", replayGain->track_peak);
    print_gain(outFile, "album gain", replayGain->album_gain);
    print_peak(outFile, "album peak", replayGain->album_peak);
}

void utilities::dumpStereo3d(QTextStream &outFile, AVPacketSideData *sideData){
    AVStereo3D *stereo;

    if (sideData->size < sizeof(*stereo)) {
        outFile << "invalid data";
        return;
    }

    stereo = (AVStereo3D *)sideData->data;

    outFile << av_stereo3d_type_name(stereo->type);

    if (stereo->flags & AV_STEREO3D_FLAG_INVERT)
        outFile <<" (inverted)";
}

void utilities::dumpAudioServiceType(QTextStream &outFile, AVPacketSideData *sideData)
{
    enum AVAudioServiceType *audioServiceType = (enum AVAudioServiceType *)sideData->data;

    if (sideData->size < sizeof(*audioServiceType)) {
        outFile << "invalid data";
        return;
    }

    switch (*audioServiceType) {
    case AV_AUDIO_SERVICE_TYPE_MAIN:
        outFile << "main";
        break;
    case AV_AUDIO_SERVICE_TYPE_EFFECTS:
        outFile << "effects";
        break;
    case AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED:
        outFile << "visually impaired";
        break;
    case AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED:
        outFile << "hearing impaired";
        break;
    case AV_AUDIO_SERVICE_TYPE_DIALOGUE:
        outFile << "dialogue";
        break;
    case AV_AUDIO_SERVICE_TYPE_COMMENTARY:
        outFile << "comentary";
        break;
    case AV_AUDIO_SERVICE_TYPE_EMERGENCY:
        outFile << "emergency";
        break;
    case AV_AUDIO_SERVICE_TYPE_VOICE_OVER:
        outFile << "voice over";
        break;
    case AV_AUDIO_SERVICE_TYPE_KARAOKE:
        outFile << "karaoke";
        break;
    default:
        outFile << "unknown";
        break;
    }
}

void utilities::dumpCPB(QTextStream &outFile, AVPacketSideData *sideData){
    AVCPBProperties *cpb = (AVCPBProperties *)sideData->data;

    if (sideData->size < sizeof(*cpb)) {
        outFile << "invalid data";
        return;
    }

    outFile << "bitrate max/min/avg: " << cpb->max_bitrate << "/" << cpb->min_bitrate << "/" << cpb->avg_bitrate;
    outFile << " buffer size: " << cpb->buffer_size << "vbv_delay: " << cpb->vbv_delay;
}

