#include "vid_ffmpeg.h"
#include "../control_bar.h"

#ifdef WITH_FFMPEG

#include <limits.h>
#if ULONG_MAX == 4294967295UL
#define UINT64_C(n) n ## ULL
#else
#define UINT64_C(n) n ## UL
#endif
#ifndef UINT64_MAX
#define UINT64_MAX (UINT64_C(18446744073709551615))
#endif
#define INT64_MAX 0x7fffffffffffffffLL
#define INT64_MIN (-INT64_MAX - 1LL)

#define RAW_OUT_ON_PLANAR true

#include "../img_handler.h"

#include "../utils/utils.h"
#include "../utils_snd/utils_snd.h"
#include "../utils_gfx/ttf.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#ifdef WITH_FFMPEG_SOUND
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#endif
}



char ffmpeg_ext[][10] = {
// "str", "aa", "aac", "aax", "ac3", "acm", "adf", "adp", "dtk", "ads", "ss2", "adx", "aea", "afc", "aix", 
// "al", "apc", "ape", "apl", "mac", "aptx", "aptxhd", "aqt", "ast", "obu", "avi", "avr", "avs", "avs2", 
// "avs3", "bfstm", "bcstm", "binka", "bit", "bitpacked", "bmv", "bonk", "brstm", "avs", "cdg", "cdxl", "xl", 
// "c2", "302", "daud", "str", "adp", "dfpwm", "dav", "dss", "dts", "dtshd", "dv", "dif", "cdata", "eac3", 
// "ec3", "paf", "fap", "flm", "flac", "flv", "fsb", "fwse", "g722", "722", "tco", "rco", "g723_1", "g729", 
// "genh", "gsm", "h261", "h26l", "h264", "264", "avc", "hca", "hevc", "h265", "265", "idf", "ifv", "cgi", 
// "ipu", "sf", "ircam", "ivr", "kux", "laf", "flv", "dat", "lvf", "m4v", "mkv", "mk3d", "mka", "mks", "webm", 
// "mca", "mcc", "mjpg", "mjpeg", "mpo", "j2k", "mlp", "mods", "moflex", "mov", "mp4", "m4a", "3gp", "3g2", 
// "mj2", "psp", "m4b", "ism", "ismv", "isma", "f4v", "avif", "mpeg", "mpg", "mp2", "mp3", "m2a", "mpa", "mpc", 
// "mjpg", "txt", "mpl2", "sub", "msf", "mtaf", "ul", "musx", "mvi", "mxg", "v", "nist", "sph", "nsp", "nut", 
// "obu", "ogg", "oma", "omg", "aa3", "pjs", "pvf", "yuv", "cif", "qcif", "rgb", "rt", "rsd", "rka", "rsd", "rso", 
// "sw", "sb", "smi", "sami", "sbc", "msbc", "sbg", "scc", "sdns", "sdr2", "sds", "sdx", "ser", "sga", "shn", 
// "vb", "son", "imx", "sln", "mjpg", "stl", "sub", "sub", "sup", "svag", "svs", "tak", "thd", "tta", "ans", "art", 
// "asc", "diz", "ice", "nfo", "txt", "vt", "ty", "ty+", "uw", "ub", "v210", "yuv10", "vag", "vc1", "rcv", "viv", 
// "idx", "vpk", "txt", "vqf", "vql", "vqe", "way", "wa", "vtt", "wsd", "xmd", "xmv", "xvag", "yop", "y4m", "ogm"
"a64", "A64", "ac3", "aac", "adts", "adx", "aif", "aiff", "afc", "aifc", "tun", "pcm", "amr", "amv", "apm", "apng", "aptx", "aptxhd",
"cvg", "asf", "wmv", "wma", "ass", "ssa", "ast", "asf", "wmv", "wma", "au", "avi", "avif", "avs", "avs2", "avs3", "bit", "caf", "cavs",
"c2", "mpd", "302", "dfpwm", "drc", "vc2", "dnxhd", "dnxhr", "dts", "dv", "eac3", "ec3", "f4v", "ffmeta", "flm", "fits", "flac", "flv",
"g722", "tco", "rco", "gif", "gsm", "gxf", "h261", "h263", "h264", "264", "hevc", "h265", "265", "m3u8", "ico", "lbc", "bmp", "dpx", "exr",
"jls", "jpeg", "jpg", "jxl", "ljpg", "pam", "pbm", "pcx", "pfm", "pgm", "pgmyuv", "phm", "png", "ppm", "sgi", "tga", "tif", "tiff", "jp2", "j2c",
"j2k", "xwd", "sun", "ras", "rs", "im1", "im8", "im24", "sunras", "vbn", "xbm", "xface", "pix", "y", "avif", "qoi", "hdr", "wbmp", "m4v", "m4a",
"m4b", "sf", "ircam", "ismv", "isma", "ivf", "jss", "js", "vag", "latm", "loas", "lrc", "m4v", "mkv", "mka", "sub", "mjpg", "mjpeg",
"mlp", "mmf", "mov", "mp2", "m2a", "mpa", "mp3", "mp4", "mpg", "mpeg", "mpg", "mpeg", "m1v", "dvd", "vob", "m2v", "vob", "ts", "m2t",
"m2ts", "mts", "mjpg", "mxf", "nut", "obu", "oga", "ogg", "ogv", "ogm", "oma", "opus", "al", "ul", "sw", "sb", "uw", "ub", "mp4", "psp", "yuv",
"rgb", "rm", "ra", "roq", "rso", "sbc", "msbc", "scc", "cpk", "sox", "spx", "spdif", "srt", "sup", "swf", "3g2", "3gp", "thd", "tta", "ttml",
"vc1", "rcv", "voc", "w64", "webm", "xml", "chk", "webp", "vtt", "aud", "wtv", "wv", "y4m"
, "wav"
};

void st_Win_FF_Refresh(int16_t this_win_handle);
void *st_Init_FF_Ctx(void *_this_win_handle);
void *st_Init_FF_Video(void *_this_win_handle);
void *st_Play_FF_Video(void *_this_win_handle);
void *st_Close_FF(void *_this_win_handle);
void st_Print_FF_Info(const AVCodec* codec, const AVCodecContext* codecCtx, int audioStreamIndex);
void* st_Win_FF_Info(void *_this_win_handle);
void st_FF_List_Codec();
#ifdef WITH_FFMPEG_SOUND
void *st_Init_FF_Audio(void *_this_win_handle);
void *st_Callback_FF_Audio(void *_this_win_handle);
void *st_Process_FF_Audio(void *_this_win_handle);
#endif

void st_Init_FF_Media(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->autoscale = FALSE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_FF_Refresh;
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    this_win->render_win = NULL;

    this_win->wi_ffmpeg = (struct_ffmpeg *)mem_alloc(sizeof(struct_ffmpeg));
    this_win->wi_ffmpeg->pFormatCtx = NULL;
    this_win->wi_ffmpeg->pCodecVideoCtx = NULL;
    this_win->wi_ffmpeg->pCodecVideoParam = NULL;
    this_win->wi_ffmpeg->pCodecVideo = NULL;
    this_win->wi_ffmpeg->pCodecAudioCtx = NULL;
    this_win->wi_ffmpeg->pCodecAudioParam = NULL;
    this_win->wi_ffmpeg->pCodecAudio = NULL;    
    this_win->wi_ffmpeg->pFrameVideo = NULL;
    this_win->wi_ffmpeg->pFrameAudio = NULL;
    this_win->wi_ffmpeg->pPacketVideo = NULL;
    this_win->wi_ffmpeg->pFrameRGBVideo = NULL;
    this_win->wi_ffmpeg->videoStream = -1;
    this_win->wi_ffmpeg->audioStream = -1;
}

void st_Win_FF_Refresh(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    st_Start_Window_Process(this_win);
    this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
    st_Limit_Work_Area(this_win);
    st_End_Window_Process(this_win);
}

void *st_Init_FF_Ctx(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    AVFormatContext     *pFormatCtx = NULL;
/* pFormatCtx */
    if(this_win->wi_ffmpeg->pFormatCtx == NULL){
        pFormatCtx = avformat_alloc_context();
        this_win->wi_ffmpeg->pFormatCtx = (void *)pFormatCtx;
        if (!pFormatCtx) {
            sprintf(alert_message, "ERROR could not allocate memory for Format Context\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
     
        st_Path_to_Linux(this_win->wi_data->path);

        if(avformat_open_input(&pFormatCtx, this_win->wi_data->path, NULL, NULL)!=0){
            sprintf(alert_message, "Can not open input file\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(avformat_find_stream_info(pFormatCtx, NULL)<0){
            sprintf(alert_message, "Can not retrieve stream information\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        for(int i = 0; i < pFormatCtx->nb_streams; i++){
            if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                this_win->wi_ffmpeg->videoStream = i;
                break;
            }
        }
        #ifdef WITH_FFMPEG_SOUND
        for(int i = 0; i< pFormatCtx->nb_streams; i++){
            if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                this_win->wi_ffmpeg->audioStream = i;
                break;
            }
        }
        if(this_win->wi_ffmpeg->audioStream != NIL && this_win->wi_ffmpeg->videoStream != NIL){
            int ret = st_form_alert_choice_2(FORM_QUESTION, (char*)"Do you think an Atari can read both A/V?", (char*)"Video", (char*)"Audio", (char*)"Both(beta)");
            if(ret == 1){
                this_win->wi_ffmpeg->audioStream = NIL;
            }
            if(ret == 2){
                this_win->wi_ffmpeg->videoStream = NIL;
            }
        }
        #endif
        avformat_seek_file(pFormatCtx, 0, 0, 0, 0, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
    }
    return NULL;
}

void *st_Init_FF_Video(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);    

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    AVCodecParameters   *pCodecVideoParam = NULL;
    AVCodec             *pCodecVideo = NULL;
    AVCodecContext      *pCodecVideoCtx = NULL;
    AVFrame             *pFrameVideo = NULL;
    AVFrame             *pFrameRGBVideo = NULL;
    AVPacket            *pPacketVideo = NULL;
    AVPixelFormat       screen_format;
    SwsContext          *sws_ctx = NULL;

    // int                 align_nb = 32;
    int                 destination_buffer_size;
    uint8_t             *destination_buffer = NULL;

    if(this_win->wi_ffmpeg->videoStream != -1){
        this_win->wi_data->video_media = true;
/* pCodecVideoParam */
        this_win->wi_ffmpeg->pCodecVideoParam = (void*)pFormatCtx->streams[this_win->wi_ffmpeg->videoStream]->codecpar;
        pCodecVideoParam = (AVCodecParameters *)this_win->wi_ffmpeg->pCodecVideoParam;     
/* pCodecVideo */
        this_win->wi_ffmpeg->pCodecVideo = (void*)avcodec_find_decoder(pCodecVideoParam->codec_id);
        pCodecVideo = (AVCodec*)this_win->wi_ffmpeg->pCodecVideo;
        if(this_win->wi_ffmpeg->pCodecVideo == NULL) {
            sprintf(alert_message, "Video\nUnsupported codec id %d!\n", pCodecVideoParam->codec_id);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pCodecVideoCtx */
        this_win->wi_ffmpeg->pCodecVideoCtx = (void*)avcodec_alloc_context3(pCodecVideo);
        pCodecVideoCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecVideoCtx;        
        if (this_win->wi_ffmpeg->pCodecVideoCtx == NULL) {
            sprintf(alert_message, "Video AVCodecContext\nFailed to allocated memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if (avcodec_parameters_to_context(pCodecVideoCtx, pCodecVideoParam) < 0) {
            sprintf(alert_message, "Video\nFailed to copy codec params to codec context\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(avcodec_open2(pCodecVideoCtx, pCodecVideo, NULL) < 0) {
            sprintf(alert_message, "Video\nCould not open codec\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pFrameVideo */
        this_win->wi_ffmpeg->pFrameVideo = (void*)av_frame_alloc();
        pFrameVideo = (AVFrame *)this_win->wi_ffmpeg->pFrameVideo;
        if (this_win->wi_ffmpeg->pFrameVideo == NULL) {
            sprintf(alert_message, "Video AVFrame\nFailed to allocate memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pFrameRGBVideo */
        this_win->wi_ffmpeg->pFrameRGBVideo = (void*)av_frame_alloc();
        pFrameRGBVideo = (AVFrame *)this_win->wi_ffmpeg->pFrameRGBVideo;
        if (this_win->wi_ffmpeg->pFrameRGBVideo == NULL) {
            sprintf(alert_message, "failed to allocate memory for AVFrame\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pPacketVideo */        
        this_win->wi_ffmpeg->pPacketVideo = (void*)av_packet_alloc();
        pPacketVideo = (AVPacket *)this_win->wi_ffmpeg->pPacketVideo;
        if (this_win->wi_ffmpeg->pPacketVideo == NULL) {
            sprintf(alert_message, "Video AVPacket\nFailed to allocate memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        this_win->wi_data->img.img_total = pFormatCtx->streams[this_win->wi_ffmpeg->videoStream]->r_frame_rate.num;
        this_win->wi_ffmpeg->fps = (double)pFormatCtx->streams[this_win->wi_ffmpeg->videoStream]->r_frame_rate.num / (double)pFormatCtx->streams[this_win->wi_ffmpeg->videoStream]->r_frame_rate.den;
        this_win->wi_ffmpeg->delay = (1 / this_win->wi_ffmpeg->fps) * 1000;        
/* screen_format */
        switch (screen_workstation_bits_per_pixel) {
            case 1:
                screen_format = AV_PIX_FMT_MONOWHITE;
                break;
            case 16:
                screen_format = AV_PIX_FMT_RGB565;
                break;
            case 24:
                screen_format = AV_PIX_FMT_RGB24;
                break;    
            case 32:
                screen_format = AV_PIX_FMT_RGB32;
                break;
            default:
                sprintf(alert_message, "screen_workstation_bits_per_pixel not supported\n");
                st_form_alert(FORM_EXCLAM, alert_message);
                break;
        }
/* destination_buffer */
        destination_buffer_size = av_image_get_buffer_size(screen_format, pCodecVideoCtx->width , pCodecVideoCtx->height , screen_workstation_bits_per_pixel);
        destination_buffer = (uint8_t *)mem_alloc( destination_buffer_size * sizeof(uint8_t) );
        memset(destination_buffer, 0, destination_buffer_size);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %dBytes", destination_buffer_size);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        av_image_fill_arrays(pFrameRGBVideo->data, pFrameRGBVideo->linesize, destination_buffer, screen_format, pCodecVideoCtx->width, pCodecVideoCtx->height, screen_workstation_bits_per_pixel);
/* SWS Context */
        this_win->wi_ffmpeg->sws_ctx = (void *)sws_getContext(pCodecVideoCtx->width, pCodecVideoCtx->height,
                                    pCodecVideoCtx->pix_fmt,
                                    pCodecVideoCtx->width, pCodecVideoCtx->height,
                                    screen_format, SWS_FAST_BILINEAR,
                                    NULL, NULL, NULL );
        sws_ctx = (SwsContext *)this_win->wi_ffmpeg->sws_ctx;
        if(this_win->wi_ffmpeg->sws_ctx == NULL){
            sprintf(alert_message, "SWS Context - Failed");
            st_form_alert(FORM_EXCLAM, alert_message);
        }        
/* Set MFDB */
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, pCodecVideoCtx->width, pCodecVideoCtx->height, screen_workstation_bits_per_pixel);
        st_Win_Set_Ready(this_win, pCodecVideoCtx->width, pCodecVideoCtx->height);
        if(this_win->wi_to_display_mfdb != NULL){
            if(this_win->wi_to_display_mfdb->fd_addr != NULL){
                st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);
            }
        } 
        send_message(this_win_handle, WM_REDRAW);        
        // av_seek_frame(pFormatCtx, this_win->wi_ffmpeg->videoStream, 0, AVSEEK_FLAG_FRAME);
    }
    return NULL;
}

#ifdef WITH_FFMPEG_SOUND
void *st_Init_FF_Audio(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    AVCodecParameters   *pCodecAudioParam = NULL;
    AVCodec             *pCodecAudio = NULL;
    AVCodecContext      *pCodecAudioCtx = NULL;
    AVFrame             *pFrameAudio = NULL;    
    AVPacket            *pPacketAudio = NULL;

    if(this_win->wi_ffmpeg->audioStream != -1){
        this_win->wi_data->sound_media = true;
/* pCodecAudioParam */            
        this_win->wi_ffmpeg->pCodecAudioParam = (void*)pFormatCtx->streams[this_win->wi_ffmpeg->audioStream]->codecpar;
        pCodecAudioParam = (AVCodecParameters *)this_win->wi_ffmpeg->pCodecAudioParam;
/* pCodecAudio */
        this_win->wi_ffmpeg->pCodecAudio = (void*)avcodec_find_decoder(pCodecAudioParam->codec_id);
        pCodecAudio = (AVCodec*)this_win->wi_ffmpeg->pCodecAudio;
        if(this_win->wi_ffmpeg->pCodecAudio == NULL) {
            sprintf(alert_message, "Audio\nUnsupported codec id %d!\n", pCodecAudioParam->codec_id);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pCodecAudioCtx */
        this_win->wi_ffmpeg->pCodecAudioCtx = (void*)avcodec_alloc_context3(pCodecAudio);
        pCodecAudioCtx = (AVCodecContext*)this_win->wi_ffmpeg->pCodecAudioCtx;
        if (pCodecAudioCtx == NULL) {
            sprintf(alert_message, "Audio AVCodecContext\nFailed to allocated memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if (avcodec_parameters_to_context(pCodecAudioCtx, pCodecAudioParam) < 0) {
            sprintf(alert_message, "Audio\nFailed to copy codec params to codec context\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(avcodec_open2(pCodecAudioCtx, pCodecAudio, NULL) < 0) {
            sprintf(alert_message, "Audio\nCould not open codec\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
/* pFrameAudio */
        this_win->wi_ffmpeg->pFrameAudio = (void*)av_frame_alloc();
        pFrameAudio = (AVFrame*)this_win->wi_ffmpeg->pFrameAudio;
        if (this_win->wi_ffmpeg->pFrameAudio == NULL) {
            sprintf(alert_message, "Audio AVFrame\nFailed to allocate memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }    
/* pPacketAudio */        
        this_win->wi_ffmpeg->pPacketAudio = (void*)av_packet_alloc();
        pPacketAudio = (AVPacket*)this_win->wi_ffmpeg->pPacketAudio;
        if (this_win->wi_ffmpeg->pPacketAudio == NULL) {
            sprintf(alert_message, "Audio AVPacket\nFailed to allocate memory\n");
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        // pPacketAudio->data = NULL;
        // pPacketAudio->size = 0;
        
        this_win->wi_snd = st_Init_Sound_Struct();
        this_win->wi_snd->original_samplerate = pCodecAudioCtx->sample_rate;
        this_win->wi_snd->original_channels = pCodecAudioCtx->ch_layout.nb_channels;
        this_win->wi_snd->effective_channels = 2;
        this_win->wi_snd->effective_bytes_per_samples = 2;
        this_win->wi_snd->effective_sampleformat =  AV_SAMPLE_FMT_S16 ;
        AVSampleFormat dst_format = (AVSampleFormat)this_win->wi_snd->effective_sampleformat;
 
        this_win->wi_snd->sound_feed = st_Process_FF_Audio;
        this_win->wi_snd->win_handle = this_win->wi_handle;

        this_win->wi_snd->original_sampleformat = pCodecAudioCtx->sample_fmt;
        this_win->wi_snd->duration_s = pFormatCtx->duration / AV_TIME_BASE;
        // this_win->wi_snd->wanted_samplerate = 32779;

        st_Preset_Snd((void*)this_win->wi_snd);

        this_win->wi_ffmpeg->swr_ctx = (void*)swr_alloc();

        SwrContext *Swr_Ctx = (SwrContext *)this_win->wi_ffmpeg->swr_ctx;
        if (!Swr_Ctx) {
            fprintf(stderr, "ERROR - Could not allocate resampler context\n");
        }
        av_opt_set_int(Swr_Ctx, "in_channel_layout", this_win->wi_snd->original_channels, 0);
        av_opt_set_int(Swr_Ctx, "out_channel_layout", this_win->wi_snd->effective_channels,  0);
        av_opt_set_int(Swr_Ctx, "in_sample_rate", this_win->wi_snd->original_samplerate, 0);
        av_opt_set_int(Swr_Ctx, "out_sample_rate", this_win->wi_snd->wanted_samplerate, 0);
        av_opt_set_sample_fmt(Swr_Ctx, "in_sample_fmt", pCodecAudioCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(Swr_Ctx, "out_sample_fmt", dst_format,  0);


        if (swr_init((SwrContext*)this_win->wi_ffmpeg->swr_ctx) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
        }

        // st_Print_FF_Info((AVCodec*)this_win->wi_ffmpeg->pCodecAudio, 
        //         (AVCodecContext*)this_win->wi_ffmpeg->pCodecAudioCtx, 
        //         this_win->wi_ffmpeg->audioStream);

        if(this_win->wi_ffmpeg->videoStream == -1){
            int16_t win_width = 300;
            int16_t win_height = 420;
/* Destination Buffer */
            u_int32_t destination_buffer_size = MFDB_STRIDE(win_width) * win_height * (screen_workstation_bits_per_pixel >> 3);
            u_int8_t *destination_buffer = (u_int8_t *)mem_alloc( destination_buffer_size );
            /* Black color */
            // memset(destination_buffer, 0, destination_buffer_size);
            /* White color */
            memset(destination_buffer, 0xFF, destination_buffer_size);
            if(destination_buffer == NULL){
                sprintf(alert_message, "Out Of Mem Error\nAsked for %dBytes", destination_buffer_size);
                st_form_alert(FORM_EXCLAM, alert_message);
            }
/* Set MFDB */
            if(this_win->wi_original_mfdb.fd_addr != NULL){
                mem_free(this_win->wi_original_mfdb.fd_addr);
            }
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, win_width, win_height, screen_workstation_bits_per_pixel);          
            st_Win_Set_Ready(this_win, win_width, win_height);
            // st_Win_FF_Info(_this_win_handle);            
            this_win->wi_data->stop_original_data_load = TRUE;
            if(this_win->wi_to_display_mfdb != NULL){
                if(this_win->wi_to_display_mfdb->fd_addr != NULL){
                    st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);
                }
            } 
            send_message(this_win_handle, WM_REDRAW);

            av_seek_frame(pFormatCtx, this_win->wi_ffmpeg->audioStream, 0, AVSEEK_FLAG_FRAME);
        }
    }
    return NULL;
}
#endif

void *st_Win_Play_FF_Media(void *_this_win_handle){

    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);

    pthread_t thread_video;
    pthread_t thread_audio;

    av_log_set_level(AV_LOG_QUIET);

    st_Init_FF_Ctx(_this_win_handle);
    st_Init_FF_Video(_this_win_handle);
    /* > AUDIO */
        #ifdef WITH_FFMPEG_SOUND
        st_Init_FF_Audio(_this_win_handle);
        #endif
    /* < AUDIO */
    this_win->refresh_win(this_win->wi_handle);

/* > AUDIO */
    #ifdef WITH_FFMPEG_SOUND
    if(this_win->wi_ffmpeg->audioStream != NIL){    
        st_Sound_Buffer_Alloc((void*)this_win->wi_snd);
        st_Win_FF_Info(_this_win_handle); 
        this_win->refresh_win(this_win->wi_handle);
        st_Init_Sound((void*)this_win->wi_snd);
        pthread_create( &thread_audio, NULL, st_Callback_FF_Audio, _this_win_handle);
    }
    #endif
/* < AUDIO */

    if(this_win->wi_ffmpeg->videoStream != NIL){
        pthread_create( &thread_video, NULL, &st_Play_FF_Video, _this_win_handle);
    }

    if(this_win->wi_ffmpeg->videoStream != NIL){
        pthread_join( thread_video, NULL);
    }

/* > AUDIO */
    #ifdef WITH_FFMPEG_SOUND
    if(this_win->wi_ffmpeg->audioStream != NIL){  
        pthread_join( thread_audio, NULL);
        st_Sound_Close((void*)this_win->wi_snd);
    }

    #endif
/* < AUDIO */
    st_Close_FF(_this_win_handle);

    send_message(this_win_handle, WM_CLOSED);

    return NULL; 

}

void *st_Close_FF(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win = detect_window(this_win_handle);

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
/* VIDEO */
    if(this_win->wi_ffmpeg->videoStream != NIL){
        AVPacket            *pPacketVideo = (AVPacket*)this_win->wi_ffmpeg->pPacketVideo;
        AVCodecContext      *pCodecVideoCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecVideoCtx;
        AVFrame             *pFrameVideo = (AVFrame*)this_win->wi_ffmpeg->pFrameVideo;
        AVFrame             *pFrameRGBVideo = (AVFrame*)this_win->wi_ffmpeg->pFrameRGBVideo;
        SwsContext          *sws_ctx = (SwsContext *)this_win->wi_ffmpeg->sws_ctx; 
        av_free(pPacketVideo);
        av_free(pFrameRGBVideo);    
        av_free(pFrameVideo);
        avcodec_close(pCodecVideoCtx); 
        sws_freeContext(sws_ctx);       
    }
/* AUDIO */
    if(this_win->wi_ffmpeg->audioStream != NIL){
        AVPacket            *pPacketAudio = (AVPacket*)this_win->wi_ffmpeg->pPacketAudio;
        AVCodecContext      *pCodecAudioCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecAudioCtx;
        AVFrame             *pFrameAudio = (AVFrame*)this_win->wi_ffmpeg->pFrameAudio;
        SwrContext          *Swr_Ctx = (SwrContext *)this_win->wi_ffmpeg->swr_ctx;
        av_free(pPacketAudio);
        av_frame_free(&pFrameAudio);
        swr_free(&Swr_Ctx);
        avcodec_close(pCodecAudioCtx);
    }
    avformat_close_input(&pFormatCtx);

    return NULL;
}

bool st_Check_FF_Ext(const char* this_ext){
    for (size_t i = 0; i < sizeof(ffmpeg_ext) / sizeof(ffmpeg_ext[0]); i++)
    {
        if (check_ext(this_ext, ffmpeg_ext[i])){
            return true;
        }
    }
    return false;
}

void *st_Play_FF_Video(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win = detect_window(this_win_handle);

    int response = 0;
    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    AVPacket            *pPacketVideo = (AVPacket*)this_win->wi_ffmpeg->pPacketVideo;
    AVCodecContext      *pCodecVideoCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecVideoCtx;
    AVFrame             *pFrameVideo = (AVFrame*)this_win->wi_ffmpeg->pFrameVideo;
    AVFrame             *pFrameRGBVideo = (AVFrame*)this_win->wi_ffmpeg->pFrameRGBVideo;
    SwsContext          *sws_ctx = (SwsContext *)this_win->wi_ffmpeg->sws_ctx;

    if(pPacketVideo == NULL){
        return NULL;
    }

    while(this_win->wi_data->wi_pth != NULL){
        while( (av_read_frame(pFormatCtx, pPacketVideo) >= 0) && this_win->wi_data->play_on && this_win->wi_data->wi_pth != NULL){
            if( pPacketVideo->stream_index == this_win->wi_ffmpeg->videoStream ){
                this_win->wi_ffmpeg->time_start = clock();
                response = avcodec_send_packet(pCodecVideoCtx, pPacketVideo);
                while (response >= 0) {
                    response = avcodec_receive_frame(pCodecVideoCtx, pFrameVideo);
                    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                        break;
                    } else if (response < 0) {
                        printf("Error while receiving a frame from the decoder: %d\n", response);
                        break;
                    }
                    sws_scale(sws_ctx, (uint8_t const * const *)pFrameVideo->data,
                    pFrameVideo->linesize, 0, pCodecVideoCtx->height,
                    pFrameRGBVideo->data, pFrameRGBVideo->linesize);
                }
                
                this_win->wi_ffmpeg->time_end = clock();
                this_win->wi_ffmpeg->duration = (double)5 * (double)(this_win->wi_ffmpeg->time_end - this_win->wi_ffmpeg->time_start);

                while( this_win->wi_ffmpeg->duration < this_win->wi_ffmpeg->delay){
                    this_win->wi_ffmpeg->duration = (double)5 * (double)(clock() - this_win->wi_ffmpeg->time_start);
                    pthread_yield_np();
                }

                st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);          
                send_message(this_win_handle, WM_REDRAW);
            }
            av_packet_unref(pPacketVideo);
            pthread_yield_np();
        }
        if(this_win->wi_data->play_on && this_win->wi_data->wi_pth != NULL){
            if(av_read_frame(pFormatCtx, pPacketVideo) < 0){
                av_seek_frame(pFormatCtx, this_win->wi_ffmpeg->videoStream, 0, AVSEEK_FLAG_BACKWARD);
            }
        }
        pthread_yield_np();
    }
    return NULL;
}

#ifdef WITH_FFMPEG_SOUND
void* st_Callback_FF_Audio(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win = detect_window(this_win_handle);

    int response = 0;

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    AVPacket            *pPacketAudio = (AVPacket*)this_win->wi_ffmpeg->pPacketAudio;

    if(pPacketAudio == NULL){
        return NULL;
    }
    while(this_win->wi_data->wi_pth != NULL){
        st_Sound_Feed((void*)this_win->wi_snd);
        pthread_yield_np();
    }

    return NULL;
}

void* st_Process_FF_Audio(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win = detect_window(this_win_handle);

    int response = 0;

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    AVPacket            *pPacketAudio = (AVPacket*)this_win->wi_ffmpeg->pPacketAudio;
    AVCodecContext      *pCodecAudioCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecAudioCtx;
    AVFrame             *pFrameAudio = (AVFrame*)this_win->wi_ffmpeg->pFrameAudio;
    SwrContext          *Swr_Ctx = (SwrContext *)this_win->wi_ffmpeg->swr_ctx;

    AVSampleFormat dst_format = (AVSampleFormat)this_win->wi_snd->effective_sampleformat;
    int ret = 0;
    uint32_t data_size = 0;
    int got_samples = 0;
    int out_samples = 2048;
    int out_channels = this_win->wi_snd->effective_channels;
    const int max_buffer_size =
        av_samples_get_buffer_size(
            NULL, out_channels, out_samples, AV_SAMPLE_FMT_S16, 1);    
    int sample_size = av_get_bytes_per_sample(pCodecAudioCtx->sample_fmt);
    uint8_t* buffer = (uint8_t*)av_malloc( max_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE ); /* Should be enought for packets */
    uint8_t *pData = (uint8_t *)this_win->wi_snd->pLogical;

    /* CONSUME SURPLUS PACKETS */
    if(this_win->wi_snd->surplus_buffer_size){
        u_int16_t* this_ptr_dst;
        u_int16_t* this_ptr_src;
        this_ptr_dst = (u_int16_t*)&pData[data_size];
        this_ptr_src = (u_int16_t*)&this_win->wi_snd->surplus_buffer[0];    
        for (int16_t i = 0; i < this_win->wi_snd->surplus_buffer_size; i++) {
            *this_ptr_dst++ = this_ptr_src[i];
        }
        data_size += this_win->wi_snd->surplus_buffer_size;
        // printf("Consumed %lu Bytes from Surplus Buffer vs %lu Bytes\n", this_win->wi_snd->surplus_buffer_size, this_win->wi_snd->bufferSize);
        this_win->wi_snd->surplus_buffer_size = 0;
    }

    while(data_size < this_win->wi_snd->bufferSize && av_read_frame(pFormatCtx, pPacketAudio) >= 0 ){

        if( pPacketAudio->stream_index == this_win->wi_ffmpeg->audioStream ){
            response = avcodec_send_packet(pCodecAudioCtx, pPacketAudio);
            while (response >= 0) {
                response = avcodec_receive_frame(pCodecAudioCtx, pFrameAudio);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    printf("Error while receiving a frame from the decoder: %d\n", response);
                    break;
                }

                // int out_samples = av_rescale_rnd(swr_get_delay(Swr_Ctx, this_win->wi_snd->original_samplerate) +
                //                      pFrameAudio->nb_samples, this_win->wi_snd->effective_samplerate, 
                //                      this_win->wi_snd->original_samplerate, AV_ROUND_UP);

                // convert input frame to output buffer
                got_samples = swr_convert(
                    Swr_Ctx,
                    &buffer, out_samples,
                    (const uint8_t **)pFrameAudio->data, pFrameAudio->nb_samples);

                if (got_samples < 0) {
                    fprintf(stderr, "error: swr_convert()\n");
                    exit(1);
                }

                while (got_samples > 0) {
                    int len_per_ch; 
                    int len = av_samples_get_buffer_size(NULL, this_win->wi_snd->effective_channels, got_samples, dst_format, 1);
                    int diff_buffer_size = data_size + len;
/* SURPLUS AUDIO PACKETS */
                    if(diff_buffer_size > this_win->wi_snd->bufferSize){

                        int diff = diff_buffer_size - this_win->wi_snd->bufferSize;
                        int new_len = len - diff;
                        u_int16_t* this_ptr_dst;
                        u_int16_t* this_ptr_src;
                        // printf("-> new_len %ld Bytes\n", new_len);
                        len_per_ch = new_len >> 1;
                        // printf("-> len_per_ch %ld Bytes\n", len_per_ch);
                        this_ptr_dst = (u_int16_t*)&pData[data_size];
                        this_ptr_src = (u_int16_t*)&buffer[0];
                        // printf("-> data_size %ld Bytes - process to reach %lu\n", data_size, data_size + (len_per_ch << 1));
                        for (int i = 0; i < len_per_ch; i++) {
                            *this_ptr_dst++ = this_ptr_src[i];
                            // printf("DBG point %d - ", i);
                            *this_ptr_dst++ = this_ptr_src[i];
                            // *this_ptr_dst++ = this_ptr_src[i + len_per_ch];
                        }
                        data_size += new_len;
                        // printf("-> new data_size %ld Bytes\n", data_size);
                        this_ptr_src = (u_int16_t*)&buffer[len_per_ch];
                        this_ptr_dst = (u_int16_t*)this_win->wi_snd->surplus_buffer;
                        // printf("-> diff %ld Bytes\n", diff);
                        if(diff){
                            len_per_ch = diff >> 1;
                            for (int i = 0; i < len_per_ch; i++) {
                                *this_ptr_dst++ = this_ptr_src[i];
                                *this_ptr_dst++ = this_ptr_src[i];
                                // *this_ptr_dst++ = this_ptr_src[i + len_per_ch];
                            }
                        }
                        this_win->wi_snd->surplus_buffer_size = diff;
                    } else {
                        u_int16_t* this_ptr_dst;
                        u_int16_t* this_ptr_src;
                        len_per_ch = len >> 1; /* Div by 2 if 2 channels - Perf opti*/
                        this_ptr_dst = (u_int16_t*)&pData[data_size];
                        this_ptr_src = (u_int16_t*)&buffer[0];
                        for (int i = 0; i < len_per_ch; i++) {
                            *this_ptr_dst++ = this_ptr_src[i];
                            *this_ptr_dst++ = this_ptr_src[i];
                            // *this_ptr_dst++ = this_ptr_src[i + len_per_ch];
                        }
                        data_size += len;
                    }

                    
                    // process samples buffered inside swr context
                    got_samples = swr_convert(Swr_Ctx, &buffer, out_samples, NULL, 0);
                    if (got_samples < 0) {
                        fprintf(stderr, "error: swr_convert()\n");
                        exit(1);
                    }
                    // if(data_size > this_win->wi_snd->bufferSize){
                    //     printf("##### data_size > this_win->wi_snd->bufferSize ####\n");
                    // }
                }
            }
        }
        av_packet_unref(pPacketAudio);
    }
    if(this_win->wi_data->play_on && this_win->wi_data->wi_pth != NULL){
        if(av_read_frame(pFormatCtx, pPacketAudio) < 0){
            av_seek_frame(pFormatCtx, this_win->wi_ffmpeg->audioStream, 0, AVSEEK_FLAG_BACKWARD);
        }
    }
    av_free(buffer);
    return NULL;
}

void st_Print_FF_Info(const AVCodec* codec, const AVCodecContext* codecCtx, int audioStreamIndex) {
    fprintf(stderr, "Codec: %s\n", codec->long_name);
    if(codec->sample_fmts != NULL) {
        fprintf(stderr, "Supported sample formats: ");
        for(int i = 0; codec->sample_fmts[i] != -1; ++i) {
            fprintf(stderr, "%s", av_get_sample_fmt_name(codec->sample_fmts[i]));
            if(codec->sample_fmts[i+1] != -1) {
                fprintf(stderr, ", ");
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "---------\n");
    fprintf(stderr, "Stream:        %7d\n", audioStreamIndex);
    fprintf(stderr, "Sample Format: %7s\n", av_get_sample_fmt_name(codecCtx->sample_fmt));
    fprintf(stderr, "Sample Rate:   %7d\n", codecCtx->sample_rate);
    fprintf(stderr, "Sample Size:   %7d\n", av_get_bytes_per_sample(codecCtx->sample_fmt));
    fprintf(stderr, "Channels:      %7d\n", codecCtx->ch_layout.nb_channels);
    fprintf(stderr, "Float Output:  %7s\n", !RAW_OUT_ON_PLANAR || av_sample_fmt_is_planar(codecCtx->sample_fmt) ? "yes" : "no");
}

void* st_Win_FF_Info(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win = detect_window(this_win_handle);

    AVFormatContext     *pFormatCtx = (AVFormatContext*)this_win->wi_ffmpeg->pFormatCtx;
    const AVCodec* codec = (AVCodec*)this_win->wi_ffmpeg->pCodecAudio;
    const AVCodecContext* codecCtx = (AVCodecContext*)this_win->wi_ffmpeg->pCodecAudioCtx;
    int audioStreamIndex = this_win->wi_ffmpeg->audioStream;

    AVDictionaryEntry *tag = NULL;

    char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
    strcpy(font_path, current_path);
    strcat(font_path, TTF_DEFAULT_PATH);

    int font_size = 12;

    int i = 0, j = 0;
    int lines = 20;
    int char_per_line = 64;
    char output_txt[lines][char_per_line];
    char temp_buffer[char_per_line] = {'\0'};

    int char_box_y = 20;
    int pos_x_txt = 10, pos_y_txt = char_box_y;

    sprintf(output_txt[i],"Codec: %s", codec->long_name );
    i++;
    if(codec->sample_fmts != NULL) {
        sprintf(output_txt[i],"Supported sample formats: " );
        for(int k = 0; codec->sample_fmts[k] != -1; ++k) {
            sprintf(temp_buffer, "%s", av_get_sample_fmt_name(codec->sample_fmts[k]));
            strcat(output_txt[i], temp_buffer);
            temp_buffer[0] = '\0';
            if(codec->sample_fmts[k+1] != -1) {
                strcat(output_txt[i], ", ");
            }
        }
        i++;
    }
        sprintf(output_txt[i],"---------" );
        i++;
        sprintf(output_txt[i], "Stream:        %7d", audioStreamIndex );
        i++;
        sprintf(output_txt[i], "Sample Format: %7s", av_get_sample_fmt_name(codecCtx->sample_fmt));
        i++;
        sprintf(output_txt[i], "Sample Rate:   %7dHz", codecCtx->sample_rate);
        i++;
        sprintf(output_txt[i], "Sample Size:   %7d", av_get_bytes_per_sample(codecCtx->sample_fmt));
        i++;
        sprintf(output_txt[i], "Channels:      %7d", codecCtx->ch_layout.nb_channels);
        i++;
        sprintf(output_txt[i], "Float Output:  %7s", !RAW_OUT_ON_PLANAR || av_sample_fmt_is_planar(codecCtx->sample_fmt) ? "yes" : "no");
        i++;
        sprintf(output_txt[i],"---------" );
        i++;
        sprintf(output_txt[i],"Track Duration %.3f Seconds", this_win->wi_snd->duration_s);
        i++;
        sprintf(output_txt[i],"Prescale = %d - BufferSize %lu Bytes", this_win->wi_snd->prescale, this_win->wi_snd->bufferSize );
        i++;
        sprintf(output_txt[i],"Direct Resampling to %dHz / %d Channels", this_win->wi_snd->wanted_samplerate, this_win->wi_snd->effective_channels );
        i++;
        sprintf(output_txt[i],"---------" );
        i++;        
        while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))){
            sprintf(output_txt[i],"%s = %s", tag->key, tag->value);
            i++;
        }

        for(j = 0; j < i; j++){
            print_ft_simple(pos_x_txt, pos_y_txt, 
                            &this_win->wi_original_mfdb, 
                            font_path, font_size, output_txt[j] );
            pos_y_txt += char_box_y;
        }

    return NULL;
}

void st_FF_List_Codec(){
FILE *fptr;

// Open a file in writing mode
fptr = fopen("filename.txt", "w");


    const AVCodec* codecs;

    // Iterate through the codecs
    void* opaque = NULL;
    const AVCodec *codec;
    const AVOutputFormat *format;
    // while( codec = av_codec_iterate(&opaque))
    // { 
    //     fprintf(stderr, "%s\n", codec->long_name);
    // }

    // Iterate through the formats
    opaque = NULL;
    while( format = av_muxer_iterate(&opaque))
    {
    fprintf(fptr, "%s\n", format->extensions);
    // if (format->codec_tag != NULL)
    // {
    //     int i = 0;

    //     AVCodecID cid = AV_CODEC_ID_MPEG1VIDEO;
    //     while (cid != AV_CODEC_ID_NONE) 
    //     {
    //         cid = av_codec_get_id(format->codec_tag, i++);
    //         fprintf(stderr, "    %d\n", cid);
    //     }
    // }
    }
// Close the file
fclose(fptr);
}

#endif
#endif /* WITH_FFMPEG */