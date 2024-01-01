#include "vid_ffmpeg.h"

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

#include "../img_handler.h"

#include "../utils/utils.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

char ffmpeg_ext[][10] = {

"str", "aa", "aac", "aax", "ac3", "acm", "adf", "adp", "dtk", "ads", "ss2", "adx", "aea", "afc", "aix", "al", "apc", "ape", "apl", "mac", "aptx", "aptxhd", "aqt", "ast", "obu", "avi", "avr", "avs", "avs2", "avs3", "bfstm", "bcstm", "binka", "bit", "bitpacked", "bmv", "bonk", "brstm", "avs", "cdg", "cdxl", "xl", "c2", "302", "daud", "str", "adp", "dfpwm", "dav", "dss", "dts", "dtshd", "dv", "dif", "cdata", "eac3", "ec3", "paf", "fap", "flm", "flac", "flv", "fsb", "fwse", "g722", "722", "tco", "rco", "g723_1", "g729", "genh", "gsm", "h261", "h26l", "h264", "264", "avc", "hca", "hevc", "h265", "265", "idf", "ifv", "cgi", "ipu", "sf", "ircam", "ivr", "kux", "laf", "flv", "dat", "lvf", "m4v", "mkv", "mk3d", "mka", "mks", "webm", "mca", "mcc", "mjpg", "mjpeg", "mpo", "j2k", "mlp", "mods", "moflex", "mov", "mp4", "m4a", "3gp", "3g2", "mj2", "psp", "m4b", "ism", "ismv", "isma", "f4v", "avif", "mpeg", "mpg", "mp2", "mp3", "m2a", "mpa", "mpc", "mjpg", "txt", "mpl2", "sub", "msf", "mtaf", "ul", "musx", "mvi", "mxg", "v", "nist", "sph", "nsp", "nut", "obu", "ogg", "oma", "omg", "aa3", "pjs", "pvf", "yuv", "cif", "qcif", "rgb", "rt", "rsd", "rka", "rsd", "rso", "sw", "sb", "smi", "sami", "sbc", "msbc", "sbg", "scc", "sdns", "sdr2", "sds", "sdx", "ser", "sga", "shn", "vb", "son", "imx", "sln", "mjpg", "stl", "sub", "sub", "sup", "svag", "svs", "tak", "thd", "tta", "ans", "art", "asc", "diz", "ice", "nfo", "txt", "vt", "ty", "ty+", "uw", "ub", "v210", "yuv10", "vag", "vc1", "rcv", "viv", "idx", "vpk", "txt", "vqf", "vql", "vqe", "way", "wa", "vtt", "wsd", "xmd", "xmv", "xvag", "yop", "y4m"
    
};

void st_Win_Video_ffmpeg(int16_t this_win_handle);
void _st_Read_ffmpeg(int16_t this_win_handle, boolean file_process);

void st_Init_ffmpeg(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Video_ffmpeg;
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    this_win->render_win = NULL;

    this_win->wi_ffmpeg = (struct_ffmpeg *)mem_alloc(sizeof(struct_ffmpeg));
    this_win->wi_ffmpeg->pFormatCtx = NULL;
    this_win->wi_ffmpeg->pCodecCtx = NULL;
    this_win->wi_ffmpeg->pCodecParam = NULL;
    this_win->wi_ffmpeg->pCodec = NULL;
    this_win->wi_ffmpeg->pFrame = NULL;
    this_win->wi_ffmpeg->pPacket = NULL;
    this_win->wi_ffmpeg->pFrameRGB = NULL;
    this_win->wi_ffmpeg->videoStream = -1;
}

void st_Win_Video_ffmpeg(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    st_Start_Window_Process(this_win);
    this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
    st_Limit_Work_Area(this_win);
    st_End_Window_Process(this_win);
}

void *st_Win_Play_ffmpeg_Video(void *_this_win_handle){

    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

if(this_win->wi_ffmpeg->videoStream == -1){

    double time_start, time_end, total_duration, duration;

    int16_t videoStream = -1;

    AVFormatContext *pFormatCtx = NULL;
    AVCodecParameters *pCodecParam = NULL;
    AVCodec *pCodec = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;

    AVFrame *pFrameRGB = NULL;

    struct SwsContext *sws_ctx = NULL;

    u_int16_t width;
    u_int16_t height;
    uint8_t *buffer = NULL;
    int numBytes;

    int response = 0;
    AVPixelFormat screen_format;
    // printf("--->screen_workstation_bits_per_pixel %d\n",screen_workstation_bits_per_pixel);
    switch (screen_workstation_bits_per_pixel)
    {
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
        goto exit_1;
        break;
    }

    if(this_win->wi_ffmpeg->pFormatCtx == NULL){
        pFormatCtx = avformat_alloc_context();
        if (!pFormatCtx) {
            sprintf(alert_message, "ERROR could not allocate memory for Format Context\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_1;
        }
     
        st_Path_to_Linux(this_win->wi_data->path);

        if(avformat_open_input(&pFormatCtx, this_win->wi_data->path, NULL, NULL)!=0){
            sprintf(alert_message, "Can not open input file\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_1;
        }
        this_win->wi_ffmpeg->pFormatCtx = (void*)pFormatCtx;
        if(avformat_find_stream_info(pFormatCtx, NULL)<0){
            sprintf(alert_message, "Can not retrieve stream information\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_2;
        }
    } else {
        pFormatCtx = (AVFormatContext *)this_win->wi_ffmpeg->pFormatCtx;
    }
    if(this_win->wi_ffmpeg->pCodecParam == NULL){
        for(int i = 0; i<pFormatCtx->nb_streams; i++){
            if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStream = i;
                break;
            }
        }
        if(videoStream == -1){
            sprintf(alert_message, "Didn't find a video stream\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_2;
        } else {
            this_win->wi_data->img.img_total = pFormatCtx->streams[videoStream]->r_frame_rate.num;
            this_win->wi_ffmpeg->fps = (double)pFormatCtx->streams[videoStream]->r_frame_rate.num / (double)pFormatCtx->streams[videoStream]->r_frame_rate.den;
            this_win->wi_ffmpeg->delay = (1 / this_win->wi_ffmpeg->fps) * 1000;
            pCodecParam = pFormatCtx->streams[videoStream]->codecpar;            
            this_win->wi_ffmpeg->pCodecParam = (void*)pCodecParam;
            this_win->wi_ffmpeg->videoStream = videoStream;
        }
    } else {
        pCodecParam = (AVCodecParameters *)this_win->wi_ffmpeg->pCodecParam;
        videoStream = this_win->wi_ffmpeg->videoStream;
    }    
    if(this_win->wi_ffmpeg->pCodec == NULL){
        pCodec = (AVCodec*)avcodec_find_decoder(pCodecParam->codec_id);
        if(pCodec == NULL) {
            sprintf(alert_message, "Unsupported codec!\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_2;
        }
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL) {
            sprintf(alert_message, "failed to allocated memory for AVCodecContext\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_3;
        }
        if (avcodec_parameters_to_context(pCodecCtx, pCodecParam) < 0) {
            sprintf(alert_message, "failed to copy codec params to codec context\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_3;
        }
        if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
            sprintf(alert_message, "Could not open codec\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_3;
        }
    } else {
        pCodec = (AVCodec*)this_win->wi_ffmpeg->pCodec;
        pCodecCtx = (AVCodecContext *)this_win->wi_ffmpeg->pCodecCtx;
    }
    if(this_win->wi_ffmpeg->pFrame == NULL){
        pFrame=av_frame_alloc();
        if (pFrame == NULL) {
            sprintf(alert_message, "failed to allocate memory for AVFrame\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_4;
        }else{
            this_win->wi_ffmpeg->pFrame = (void*)pFrame;
        }
    } else {
        pFrame = (AVFrame*)this_win->wi_ffmpeg->pFrame;
    }
    if(this_win->wi_ffmpeg->pFrameRGB == NULL){
        pFrameRGB=av_frame_alloc();
        if (pFrameRGB == NULL) {
            sprintf(alert_message, "failed to allocate memory for AVFrame\n");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto exit_5;
        }else{
            this_win->wi_ffmpeg->pFrameRGB = (void*)pFrameRGB;
        }
    } else {
        pFrameRGB = (AVFrame*)this_win->wi_ffmpeg->pFrameRGB;
    }
    if(this_win->wi_ffmpeg->pPacket == NULL){
        pPacket = av_packet_alloc();
            if (pPacket == NULL) {
                sprintf(alert_message, "failed to allocate memory for AVPacket\n");
                st_form_alert(FORM_EXCLAM, alert_message);
                goto exit_5;
            }else{
                this_win->wi_ffmpeg->pPacket = (void*)pPacket;
            }
        } else {
            pPacket = (AVPacket*)this_win->wi_ffmpeg->pPacket;
    }
    /* You should first get width and height in order to build the destination buffer */
    width = pCodecParam->width;
    height = pCodecParam->height;
    /* Determine required buffer size and allocate buffer */
    numBytes = av_image_get_buffer_size(screen_format, pCodecCtx->width + 1, pCodecCtx->height + 1, 1);
    buffer = (uint8_t *)mem_alloc( numBytes * sizeof(uint8_t) );
    memset(buffer, 0, numBytes);
    if(buffer == NULL){
        sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * (screen_workstation_bits_per_pixel >> 2));
        st_form_alert(FORM_EXCLAM, alert_message);
        goto exit_5;
    }
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, screen_format, pCodecCtx->width, pCodecCtx->height, 1);

    /* Initialize SWS context for software scaling */ 
    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                pCodecCtx->pix_fmt,
                                pCodecCtx->width, pCodecCtx->height,
                                screen_format, SWS_BILINEAR,
                                NULL, NULL, NULL );
    if(this_win->wi_original_mfdb.fd_addr != NULL){
        mem_free(this_win->wi_original_mfdb.fd_addr);
    }
    mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)buffer, width, height, screen_workstation_bits_per_pixel);
    st_Win_Set_Ready(this_win, width, height);
    this_win->refresh_win(this_win->wi_handle);
    if((av_read_frame(pFormatCtx, pPacket) < 0) && videoStream != -1){
        av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BYTE);
    }       
    while(this_win->wi_data->wi_pth != NULL){
        while( (av_read_frame(pFormatCtx, pPacket) >= 0) && this_win->wi_data->play_on && this_win->wi_data->wi_pth != NULL){
            if( pPacket->stream_index == videoStream){
                time_start = clock();
                response = avcodec_send_packet(pCodecCtx, pPacket);
                if (response < 0){
                    break;
                }
                while (response >= 0) {
                    // Return decoded output data (into a frame) from a decoder
                    response = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                        break;
                    } else if (response < 0) {
                        printf("Error while receiving a frame from the decoder: %d\n", response);
                        break;
                    }
                    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    pFrameRGB->data, pFrameRGB->linesize);
                    av_packet_unref(pPacket);
                }
                time_end = clock();
                duration = (double)5 * (double)(time_end - time_start);

                while( duration <  this_win->wi_ffmpeg->delay){
                    duration = (double)5 * (double)(clock() - time_start);
                }
                st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);          
                send_message(this_win_handle, WM_REDRAW);
                pthread_yield_np();
                /*This is useful if you want to move or resize the movie in Xaaes while it's playing */
                // pthread_yield_np(); 
            }
        }
        if(this_win->wi_data->play_on && this_win->wi_data->wi_pth != NULL){
            if((av_read_frame(pFormatCtx, pPacket) < 0) && videoStream != -1){
                av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
            }
        }
        pthread_yield_np();
    }
    exit_5:
        // Free the RGB image
        av_free(pFrameRGB);    
    exit_4:
        av_free(pFrame);
    exit_3:
        // Close the codecs
        avcodec_close(pCodecCtx);
    exit_2:
        // Close the video file
        avformat_close_input(&pFormatCtx);
    exit_1:
        send_message(this_win_handle, WM_CLOSED);
}
send_message(this_win_handle, WM_CLOSED);
return NULL; 
}

bool st_check_ffmpeg_ext(const char* this_ext){
    for (size_t i = 0; i < sizeof(ffmpeg_ext) / sizeof(ffmpeg_ext[0]); i++)
    {
        if (check_ext(this_ext, ffmpeg_ext[i])){
            return true;
        }
    }
    return false;
}

#endif /* WITH_FFMPEG */