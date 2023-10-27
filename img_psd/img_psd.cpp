#include "img_psd.h"
#include "../img_handler.h"
#include "../utils_gfx/pix_convert.h"
#include "../thumbs/thumbs.h"
#include "../utils_gfx/ttf.h"
#include "../utils/utils.h"
// #include <libpsd/libpsd.h>

// the main include that always needs to be included in every translation unit that uses the PSD library
#include <Psd/Psd.h>

// for convenience reasons, we directly include the platform header from the PSD library.
// we could have just included <Windows.h> as well, but that is unnecessarily big, and triggers lots of warnings.
#include <Psd/PsdPlatform.h>

// in the sample, we use the provided malloc allocator for all memory allocations. likewise, we also use the provided
// native file interface.
// in your code, feel free to use whatever allocator you have lying around.
#include <Psd/PsdMallocAllocator.h>
#include <Psd/PsdNativeFile.h>

#include <Psd/PsdDocument.h>
#include <Psd/PsdColorMode.h>
#include <Psd/PsdLayer.h>
#include <Psd/PsdChannel.h>
#include <Psd/PsdChannelType.h>
#include <Psd/PsdLayerMask.h>
#include <Psd/PsdVectorMask.h>
#include <Psd/PsdLayerMaskSection.h>
#include <Psd/PsdImageDataSection.h>
#include <Psd/PsdImageResourcesSection.h>
#include <Psd/PsdParseDocument.h>
#include <Psd/PsdParseLayerMaskSection.h>
#include <Psd/PsdParseImageDataSection.h>
#include <Psd/PsdParseImageResourcesSection.h>
#include <Psd/PsdLayerCanvasCopy.h>
#include <Psd/PsdInterleave.h>
#include <Psd/PsdPlanarImage.h>
#include <Psd/PsdExport.h>
#include <Psd/PsdExportDocument.h>

#include <wchar.h>

#ifndef PRIMARY_IMAGE_ID
#define PRIMARY_IMAGE_ID    -1
#endif

PSD_USING_NAMESPACE;
#define CHANNEL_NOT_FOUND UINT_MAX
// #define CHANNEL_NOT_FOUND UINT_MAX

/*
PSD Functions
*/
unsigned int FindChannel(Layer* layer, int16_t channelType);

bool st_Expand_PSD_Layer(Document* document, Layer* layer, MallocAllocator* allocator, u_int8_t* destination_buffer);
bool st_Extract_PSD_Layer(Document* document, Layer* layer, MallocAllocator* allocator, u_int8_t* destination_buffer);
bool st_Extract_PSD_Image(Document* document, ImageDataSection* imageData, MallocAllocator* allocator, u_int8_t* destination_buffer, bool hasTransparencyMask);
void st_Extract_ARGB_PSD(Layer* layer, uint8_t* maskCanvasData, uint8_t* maskCanvasDataV, u_int32_t* ptr_argb, u_int32_t* imgdata, u_int16_t width, u_int16_t height, bool expanded);

void st_Win_Print_PSD(int16_t this_win_handle);
void _st_Read_PSD(int16_t this_win_handle, boolean file_process, long img_id);
void _st_Handle_Thumbs_PSD(int16_t this_win_handle, boolean file_process);
void _st_Handle_Thumbs_PSD_Generic(int16_t this_win_handle, boolean file_process);

void st_Init_PSD(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
    this_win->prefers_file_instead_mem = TRUE;
	this_win->refresh_win = st_Win_Print_PSD;
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
    /* thumbnails stuff */
    if(this_win->wi_thumb == NULL){
        if(cpu_type < 40){
            _st_Handle_Thumbs_PSD_Generic(this_win->wi_handle, this_win->prefers_file_instead_mem);
        }else{
            _st_Handle_Thumbs_PSD(this_win->wi_handle, this_win->prefers_file_instead_mem);
        }
    }
}

void st_Win_Print_PSD(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_PSD(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

/* C++ functions taken from PsdSamples.cpp */
unsigned int FindChannel(Layer* layer, int16_t channelType) {
    for (unsigned int i = 0; i < layer->channelCount; ++i) {
        Channel* channel = &layer->channels[i];
        if (channel->data && channel->type == channelType){
            return i;
            }
    }
    return CHANNEL_NOT_FOUND;
}

namespace
{
    template <typename T, typename DataHolder>
    static void* ExpandChannelToCanvas(Allocator* allocator, const DataHolder* layer, const void* data, unsigned int canvasWidth, unsigned int canvasHeight)
    {
        T* canvasData = static_cast<T*>(mem_alloc(sizeof(T)*canvasWidth*canvasHeight));
        memset(canvasData, 0u, sizeof(T)*canvasWidth*canvasHeight);

        imageUtil::CopyLayerData(static_cast<const T*>(data), canvasData, layer->left, layer->top, layer->right, layer->bottom, canvasWidth, canvasHeight);

        return canvasData;
    }

    static void* ExpandChannelToCanvas(const Document* document, Allocator* allocator, Layer* layer, Channel* channel)
    {
        if (document->bitsPerChannel == 8)
            return ExpandChannelToCanvas<uint8_t>(allocator, layer, channel->data, document->width, document->height);
        else if (document->bitsPerChannel == 16)
            return ExpandChannelToCanvas<uint16_t>(allocator, layer, channel->data, document->width, document->height);
        else if (document->bitsPerChannel == 32)
            return ExpandChannelToCanvas<float32_t>(allocator, layer, channel->data, document->width, document->height);

        return NULL;
    }

    template <typename T>
    T* CreateInterleavedImage(Allocator* allocator, const void* srcR, const void* srcG, const void* srcB, unsigned int width, unsigned int height)
    {
        T* image = static_cast<T*>(mem_alloc(width*height * 4u*sizeof(T)));

        const T* r = static_cast<const T*>(srcR);
        const T* g = static_cast<const T*>(srcG);
        const T* b = static_cast<const T*>(srcB);
        imageUtil::InterleaveRGB(r, g, b, T(0), image, width, height);

        return image;
    }

    template <typename T>
    T* CreateInterleavedImage(Allocator* allocator, const void* srcR, const void* srcG, const void* srcB, const void* srcA, unsigned int width, unsigned int height)
    {
        T* image = static_cast<T*>(mem_alloc(width*height * 4u*sizeof(T)));
        const T* r = static_cast<const T*>(srcR);
        const T* g = static_cast<const T*>(srcG);
        const T* b = static_cast<const T*>(srcB);
        const T* a = static_cast<const T*>(srcA);
        imageUtil::InterleaveRGBA(r, g, b, a, image, width, height);

        return image;
    }

    template <typename T>
    static void* ExpandMaskToCanvas(const Document* document, Allocator* allocator, T* mask)
    {
        if (document->bitsPerChannel == 8)
            return ExpandChannelToCanvas<uint8_t>(allocator, mask, mask->data, document->width, document->height);
        else if (document->bitsPerChannel == 16)
            return ExpandChannelToCanvas<uint16_t>(allocator, mask, mask->data, document->width, document->height);
        else if (document->bitsPerChannel == 32)
            return ExpandChannelToCanvas<float32_t>(allocator, mask, mask->data, document->width, document->height);

        return NULL;
    }
    /* End - C++ functions taken from PsdSamples.cpp */
}

void st_Extract_ARGB_PSD(Layer* layer, uint8_t* maskCanvasData, uint8_t* maskCanvasDataV, u_int32_t* ptr_argb, u_int32_t* imgdata, u_int16_t width, u_int16_t height, bool expanded){
    long ii, jj, kk, x, y;
        if(layer->layerMask || layer->vectorMask) {
            if(layer->layerMask){            
                if(!expanded){                      
                    int maskw = (layer->layerMask->right - layer->layerMask->left);
                    int maskh = (layer->layerMask->bottom - layer->layerMask->top);
                    int offset_x = layer->layerMask->left - layer->left > 0 ? layer->layerMask->left - layer->left : layer->left - layer->layerMask->left;
                    int offset_y = layer->layerMask->bottom - layer->bottom > 0 ? layer->layerMask->bottom - layer->bottom :  layer->bottom - layer->layerMask->bottom;
                    for(y = 0; y < height; y++){
                        for(x = 0; x < width; x++){
                            ii = (x + (y * MFDB_STRIDE(width)));
                            jj = ((y * width) + x);
                            kk = (((y - offset_y) * maskw) + (x - offset_x));
                            if(x >= offset_x && x < (maskw + offset_x) && y >= offset_y && y < (maskh + offset_y)){
                                ptr_argb[ii] = maskCanvasData[kk] == 255 ? st_Blend_Pix(ptr_argb[ii], imgdata[jj]) : ptr_argb[ii];
                            }else{
                                ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
                            }                            
                        }
                    }
                } else {
                    for(y = 0; y < height; y++){
                        for(x = 0; x < width; x++){
                            ii = (x + (y * MFDB_STRIDE(width)));
                            jj = ((y * width) + x);
                            ptr_argb[ii] = maskCanvasData[jj] == 255 ? st_Blend_Pix(ptr_argb[ii], imgdata[jj]) : ptr_argb[ii];
                        }
                    }
                }
            }
            if(layer->vectorMask){
                if(!expanded){
                    int maskw = (layer->vectorMask->right - layer->vectorMask->left);
                    int maskh = (layer->vectorMask->bottom - layer->vectorMask->top);
                    int offset_x = layer->vectorMask->left - layer->left > 0 ? layer->vectorMask->left - layer->left : layer->left - layer->vectorMask->left;
                    int offset_y = layer->vectorMask->bottom - layer->bottom > 0 ? layer->vectorMask->bottom - layer->bottom :  layer->bottom - layer->vectorMask->bottom;        
                    for(y = 0; y < height; y++){
                        for(x = 0; x < width; x++){
                            ii = (x + (y * MFDB_STRIDE(width)));
                            jj = ((y * width) + x);
                            kk = (((y - offset_y) * maskw) + (x - offset_x));
                            if(x >= offset_x && x < (maskw + offset_x) && y >= offset_y && y < (maskh + offset_y)){
                                ptr_argb[ii] = maskCanvasDataV[kk] == 255 ? st_Blend_Pix(ptr_argb[ii], imgdata[jj]) : ptr_argb[ii] ;
                            }
                        }
                    }
                } else {
                    for(y = 0; y < height; y++){
                        for(x = 0; x < width; x++){
                            ii = (x + (y * MFDB_STRIDE(width)));
                            jj = ((y * width) + x);                           
                            ptr_argb[ii] = maskCanvasDataV[jj] == 255 ? st_Blend_Pix(ptr_argb[ii], imgdata[jj]) : ptr_argb[ii] ;
                        }
                    }                    
                }
            }
        }else{
            for(y = 0; y < height; y++){
                for(x = 0; x < width; x++){
                    ii = (x + (y * MFDB_STRIDE(width)));
                    jj = ((y * width) + x);                           
                    ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
                }
            }
        }                
}

void _st_Read_PSD(int16_t this_win_handle, boolean file_process, long img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){

		u_int16_t width = MIN_WINDOWS_WSIZE; /* Set something by default */
		u_int16_t height = MIN_WINDOWS_HSIZE;
        u_int8_t* destination_buffer;
        u_int32_t max_layer;

        MallocAllocator allocator;
        NativeFile file(&allocator);
        if (!file.OpenRead(st_Char_to_WChar(this_win->wi_data->path))) {
                sprintf(alert_message, "Can't load this PSD file");
                st_form_alert(FORM_STOP, alert_message);
        }
        Document* document = CreateDocument(&file, &allocator);
        if (!document) {
            sprintf(alert_message, "Can't load this PSD file");
            st_form_alert(FORM_STOP, alert_message);        
            file.Close();
        }
        ImageDataSection* imageData = ParseImageDataSection(document, &file, &allocator);
        LayerMaskSection* layerMaskSection = ParseLayerMaskSection(document, &file, &allocator);
        bool hasTransparencyMask = layerMaskSection->hasTransparencyMask;
        if(img_id == PRIMARY_IMAGE_ID){
            img_id = 0;
            max_layer = layerMaskSection->layerCount;
            width = document->width;
            height = document->height;
            destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
            if(destination_buffer == NULL){
                sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
                st_form_alert(FORM_EXCLAM, alert_message);
                goto destroy_doc;
            }
            if(this_win->wi_original_mfdb.fd_addr != NULL){
                mem_free(this_win->wi_original_mfdb.fd_addr);
            }
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
            st_MFDB_Fill_bpp(&this_win->wi_original_mfdb, 0x00FFFFFF, 32);
            if(!document){
                goto end;
            }
            
            if(!st_Extract_PSD_Image(document, imageData, &allocator,destination_buffer, hasTransparencyMask)){

                if(st_form_alert_choice(FORM_QUESTION, (char*)"Can't handle to extract image data\nTrying to process layer by layer?", (char*)"No", (char*)"Yes") == 1){
                    goto destroy_doc;
                }                
                for (int i = img_id; i < max_layer; ++i) {
                    if(layerMaskSection) {
                        Layer* layer = &layerMaskSection->layers[i];
                        ExtractLayer(document, &file, &allocator, layer);
                        bool layer_visible = layer->isVisible;
                        if(layer_visible){
                            st_Expand_PSD_Layer(document, layer, &allocator, destination_buffer);
                        }
                    }
                } /* End 'for' layers loop */
            }
        }else{
            img_id = this_win->wi_data->img.img_id;
            if(layerMaskSection) {
                Layer* layer = &layerMaskSection->layers[img_id];
                ExtractLayer(document, &file, &allocator, layer);
                bool layer_visible = layer->isVisible;            

                const int left = layer->left > 0 ? layer->left : 0;
                const int top = layer->top > 0 ? layer->top : 0;
                const int right = layer->right < document->width ? layer->right : document->width;
                const int bottom = layer->bottom < document->height ? layer->bottom : document->height;

                width = right - left;
                height = bottom - top;
                if(width == 0 | height == 0){
                    sprintf(alert_message, "Can not parse layer %d", img_id);
                    st_form_alert(FORM_EXCLAM, alert_message);
                    goto destroy_doc;
                }
                destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
                if(destination_buffer == NULL){
                    sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
                    st_form_alert(FORM_EXCLAM, alert_message);
                    goto destroy_doc;
                }
                if(this_win->wi_original_mfdb.fd_addr != NULL){
                    mem_free(this_win->wi_original_mfdb.fd_addr);
                }
                mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);

                st_Extract_PSD_Layer(document, layer, &allocator, destination_buffer);
            }
        }
destroy_doc:
        DestroyImageDataSection(imageData, &allocator);
        DestroyLayerMaskSection(layerMaskSection, &allocator);
        DestroyDocument(document, &allocator);
        file.Close();
end:        
        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;		
	}
}

bool st_Expand_PSD_Layer(Document* document, Layer* layer, MallocAllocator* allocator, u_int8_t* destination_buffer ){

    unsigned int indexA, indexR, indexG, indexB;

    u_int8_t*   image8  = NULL;
    u_int32_t*  imgdata = NULL ;

    bool ret_value = false;
    void* canvasData[4] = {};
    u_int16_t channelCount = 0u;

    indexA = FindChannel(layer, channelType::TRANSPARENCY_MASK);
    if (indexA != CHANNEL_NOT_FOUND) {
        canvasData[0] = ExpandChannelToCanvas(document, allocator, layer, &layer->channels[indexA]);
        channelCount += 1;
    }
    indexR = FindChannel(layer, channelType::R);
    if (indexR != CHANNEL_NOT_FOUND){
        canvasData[1] = ExpandChannelToCanvas(document, allocator, layer, &layer->channels[indexR]);
        channelCount += 1;
    }
    indexG = FindChannel(layer, channelType::G);
    if (indexG != CHANNEL_NOT_FOUND){
        canvasData[2] = ExpandChannelToCanvas(document, allocator, layer, &layer->channels[indexG]);
        channelCount += 1;
    }
    indexB = FindChannel(layer, channelType::B);
    if (indexB != CHANNEL_NOT_FOUND){
        canvasData[3] = ExpandChannelToCanvas(document, allocator, layer, &layer->channels[indexB]);
        channelCount += 1;
    }
    if(document->colorMode == colorMode::RGB){
        switch (channelCount)
        {   
        case 3u:
            switch (document->bitsPerChannel)
            {
            case 8:
            /* Return RGBA */
                image8 = CreateInterleavedImage<uint8_t>(allocator, canvasData[1], canvasData[2], canvasData[3], document->width, document->height);
                imgdata = (uint32_t*)st_Convert_RGBA_to_ARGB(image8, document->width, document->height);
                break;
            default:
                sprintf(alert_message, "Not supported\nChannel(s) %d\n%dbits/channel",channelCount, document->bitsPerChannel);
                st_form_alert(FORM_EXCLAM, alert_message);                            
                break;
            }
            break;
        case 4u:
            switch (document->bitsPerChannel)
            {
            case 8:
                image8 = CreateInterleavedImage<uint8_t>(allocator, canvasData[1], canvasData[2], canvasData[3], canvasData[0], document->width, document->height);
                imgdata = (uint32_t*)st_Convert_RGBA_to_ARGB(image8, document->width, document->height);
                break;
            default:
                sprintf(alert_message, "Not supported\nChannel(s) %d\n%dbits/channel",channelCount, document->bitsPerChannel);
                st_form_alert(FORM_EXCLAM, alert_message);
                break;
            }
            break;                   
        default:
            sprintf(alert_message, "Not supported\nChannel(s) %d\nSpecified channels %d",channelCount, layer->channelCount);
            st_form_alert(FORM_EXCLAM, alert_message);
            break;
        }
    }
    mem_free(canvasData[0]);
    mem_free(canvasData[1]);
    mem_free(canvasData[2]);
    mem_free(canvasData[3]);
    if(channelCount >= 3){
        uint8_t* maskCanvasData = (uint8_t*)ExpandMaskToCanvas(document, allocator, layer->layerMask);
        uint8_t* maskCanvasDataV = (uint8_t*)ExpandMaskToCanvas(document, allocator, layer->vectorMask);
        u_int32_t* ptr_argb = (u_int32_t*)destination_buffer;

        st_Extract_ARGB_PSD(layer, maskCanvasData, maskCanvasDataV, (u_int32_t*)destination_buffer, imgdata, document->width, document->height, true);
        ret_value = true;
        mem_free(maskCanvasData);
        mem_free(maskCanvasDataV);        
    }
    mem_free(image8);
    mem_free(imgdata);
    return ret_value;
}

bool st_Extract_PSD_Layer(Document* document, Layer* layer, MallocAllocator* allocator, u_int8_t* destination_buffer ){

    const int left = layer->left > 0 ? layer->left : 0;
    const int top = layer->top > 0 ? layer->top : 0;
    const int right = layer->right < document->width ? layer->right : document->width;
    const int bottom = layer->bottom < document->height ? layer->bottom : document->height;
    u_int16_t lwidth = right - left;
    u_int16_t lheight = bottom - top;

    unsigned int indexA, indexR, indexG, indexB;

    u_int8_t*   image8  = NULL;
    u_int32_t*  imgdata = NULL;

    bool ret_value = false;
    void* canvasData[4] = {};
    u_int16_t channelCount = 0u;

    indexA = FindChannel(layer, channelType::TRANSPARENCY_MASK);
    if (indexA != CHANNEL_NOT_FOUND) {
        canvasData[0] = layer->channels[indexA].data;
        channelCount += 1;
    }
    indexR = FindChannel(layer, channelType::R);
    if (indexR != CHANNEL_NOT_FOUND){
        canvasData[1] = layer->channels[indexR].data;
        channelCount += 1;
    }
    indexG = FindChannel(layer, channelType::G);
    if (indexG != CHANNEL_NOT_FOUND){
        canvasData[2] = layer->channels[indexG].data;
        channelCount += 1;
    }
    indexB = FindChannel(layer, channelType::B);
    if (indexB != CHANNEL_NOT_FOUND){
        canvasData[3] = layer->channels[indexB].data;
        channelCount += 1;
    }

    if(document->colorMode == colorMode::RGB){
        switch (channelCount)
        {         
        case 3u:
            switch (document->bitsPerChannel)
            {
            case 8:
            /* Return RGBA */
                image8 = CreateInterleavedImage<uint8_t>(allocator, canvasData[1], canvasData[2], canvasData[3], lwidth, lheight);
                imgdata = (uint32_t*)st_Convert_RGBA_to_ARGB(image8, lwidth, lheight);
                break;
            default:
                sprintf(alert_message, "Not supported\nChannel(s) %d\n%dbits/channel",channelCount, document->bitsPerChannel);
                st_form_alert(FORM_EXCLAM, alert_message);                            
                break;
            }
            break;
        case 4u:
            switch (document->bitsPerChannel)
            {
            case 8:
                image8 = CreateInterleavedImage<uint8_t>(allocator, canvasData[1], canvasData[2], canvasData[3], canvasData[0], lwidth, lheight);
                imgdata = (uint32_t*)st_Convert_RGBA_to_ARGB(image8, lwidth, lheight);
                break;
            default:
                sprintf(alert_message, "Not supported\nChannel(s) %d\n%dbits/channel",channelCount, document->bitsPerChannel);
                st_form_alert(FORM_EXCLAM, alert_message);
                break;
            }
            break;                   
        default:
            // sprintf(alert_message, "Not supported\nChannel(s) %d\nSpecified channels %d",channelCount, layer->channelCount);
            // st_form_alert(FORM_EXCLAM, alert_message);
            break;
        }
    }
    mem_free(canvasData[0]);
    mem_free(canvasData[1]);
    mem_free(canvasData[2]);
    mem_free(canvasData[3]);
    if(channelCount >= 3){
        uint8_t* maskCanvasData = (uint8_t*)layer->layerMask->data;
        uint8_t* maskCanvasDataV = (uint8_t*)layer->vectorMask->data;
        u_int32_t* ptr_argb = (u_int32_t*)destination_buffer;

        st_Extract_ARGB_PSD(layer, maskCanvasData, maskCanvasDataV, (u_int32_t*)destination_buffer, imgdata, lwidth, lheight, false);
        ret_value = true;
    }
    mem_free(image8);
    mem_free(imgdata);
    return ret_value;
}

bool st_Extract_PSD_Image(Document* document, ImageDataSection* imageData, MallocAllocator* allocator, u_int8_t* destination_buffer, bool hasTransparencyMask ){

    bool ret_value = false;

    u_int8_t*   image8  = NULL;

	if (document->imageDataSection.length != 0) {
        if (imageData) {
			bool isRgb = imageData->imageCount == 3 ? true : false;
			if (imageData->imageCount >= 4) { isRgb = hasTransparencyMask ? false : true; }
            if(imageData->imageCount >= 3){
                if (document->bitsPerChannel == 8) {
                    if (isRgb) {
                        image8 = CreateInterleavedImage<uint8_t>(allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, document->width, document->height);
                    } else { 
                        image8 = CreateInterleavedImage<uint8_t>(allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, imageData->images[3].data, document->width, document->height);
                    }
                    uint32_t* dst_ptr = (u_int32_t*)destination_buffer;
                    u_int32_t* source_data = (u_int32_t*)image8;
                    for(int16_t y = 0; y < document->height; y++ ){
                        for(int16_t x = 0; x < document->width; x++){
                            dst_ptr[(y * MFDB_STRIDE(document->width)) + x] = ((source_data[(y * document->width) + x] & 0x000000FF) << 24 ) | ((source_data[(y * document->width) + x] & 0xFFFFFF00) >> 8);
                        }
                    }
                    ret_value = true;
                }
            }
            mem_free(image8);
		}
	}
    return ret_value;
}

/* PSD Writes */

void st_Write_PSD(u_int8_t* src_buffer, int width, int height, const char* filename) {
		MallocAllocator allocator;
		NativeFile file(&allocator);

        /* Progress Bar Stuff */
        struct_progress_bar* wi_progress_bar = (struct_progress_bar*)mem_alloc(sizeof(struct_progress_bar));
        wi_progress_bar->progress_bar_enabled = TRUE;
        wi_progress_bar->progress_bar_in_use = FALSE;
        wi_progress_bar->progress_bar_locked = FALSE;
        st_Progress_Bar_Add_Step(wi_progress_bar);
        st_Progress_Bar_Init(wi_progress_bar, (int8_t*)"PSD WRITING");
        st_Progress_Bar_Signal(wi_progress_bar, 10, (int8_t*)"Psd export init");

		// try opening the file. if it fails, bail out.
		if (!file.OpenWrite(st_Char_to_WChar(filename))) {
            sprintf(alert_message, "Can't write on disk");
            st_form_alert(FORM_STOP, alert_message);
            return;
		}
        
		// write an RGB PSD file, 8-bit
		ExportDocument* document = CreateExportDocument(&allocator, width, height, 8u, exportColorMode::RGB); {
			AddMetaData(document, &allocator, "Saved with", THIS_APP_NAME);

			const unsigned int layer1 = AddLayer(document, &allocator, "Main Picture layer");
            u_int32_t size = width * height;
            u_int8_t *src_red = (u_int8_t*)mem_alloc(size), *src_green = (u_int8_t*)mem_alloc(size), *src_blue = (u_int8_t*)mem_alloc(size);
            u_int8_t *src_alpha = (u_int8_t*)mem_alloc(size);
            st_Progress_Bar_Signal(wi_progress_bar, 35, (int8_t*)"RGB planar computing");
            for(int y = 0; y < height; y++){
                for(int x = 0; x < width; x++){
                    int i = (width * y) + x;
                    int j = ((MFDB_STRIDE(width) * y) + x) << 2;
                    src_alpha[i] = src_buffer[j++];
                    src_red[i] = src_buffer[j++];
                    src_green[i] = src_buffer[j++];
                    src_blue[i] = src_buffer[j++];
                }                
            }
            st_Progress_Bar_Signal(wi_progress_bar, 65, (int8_t*)"Updating RGB layer");
			UpdateLayer(document, &allocator, layer1, exportChannel::RED, 0, 0, width, height, &src_red[0], compressionType::RLE);
			UpdateLayer(document, &allocator, layer1, exportChannel::GREEN, 0, 0, width, height, &src_green[0], compressionType::RLE);
			UpdateLayer(document, &allocator, layer1, exportChannel::BLUE, 0, 0, width, height, &src_blue[0], compressionType::RLE);
			UpdateLayer(document, &allocator, layer1, exportChannel::ALPHA, 0, 0, width, height, &src_alpha[0], compressionType::RLE);
            UpdateMergedImage(document, &allocator, &src_red[0], &src_green[0], &src_blue[0]);
			WriteDocument(document, &allocator, &file);
            mem_free(src_red); mem_free(src_green); mem_free(src_blue); mem_free(src_alpha);
		}

		DestroyExportDocument(document, &allocator);
		file.Close();
        st_Progress_Bar_Signal(wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(wi_progress_bar);
        st_Progress_Bar_Finish(wi_progress_bar);        
}

/* THUMBS */

void _st_Handle_Thumbs_PSD(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

    u_int16_t idx = 0;

    MallocAllocator allocator;
    NativeFile file(&allocator);
    if (!file.OpenRead(st_Char_to_WChar(this_win->wi_data->path))) {
        sprintf(alert_message, "Can't load this PSD file");
        st_form_alert(FORM_STOP, alert_message);
    }
    Document* document = CreateDocument(&file, &allocator);
    if (!document) {
        sprintf(alert_message, "Can't load this PSD file");
        st_form_alert(FORM_STOP, alert_message);        
        file.Close();
    }

    LayerMaskSection* layerMaskSection = ParseLayerMaskSection(document, &file, &allocator);
    bool hasTransparencyMask = layerMaskSection->hasTransparencyMask;

    this_win->wi_data->img.img_total = layerMaskSection->layerCount;
    this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;

    if(document->colorMode == colorMode::RGB /*&& document->colorMode != colorMode::GRAYSCALE*/ && layerMaskSection->layerCount > 1){
        if(st_form_alert_choice(FORM_QUESTION, (char*)"Do you want to try to extract layers", (char*)"No", (char*)"Yes") == 1){
            this_win->wi_data->thumbnail_slave = false;
            DestroyLayerMaskSection(layerMaskSection, &allocator);
            DestroyDocument(document, &allocator);
            file.Close();                 
            return;
        }
    }else{
        return;
    }
    if(this_win->wi_data->img.img_total > 1){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"Thumbs processing");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 1, (int8_t*)"Init");

        u_int16_t final_width = document->width;
        u_int16_t final_height = document->height;

        u_int16_t wanted_width = 80;
        u_int16_t wanted_height = 100;
        if(final_height < final_width){
            wanted_width = 100;
            wanted_height = 80;
        }
        u_int16_t wanted_padx = 8;
        u_int16_t wanted_pady = 8;

        this_win->wi_data->thumbnail_slave = true;
        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, wanted_padx, wanted_pady, wanted_width, wanted_height);
 
        this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
        struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
        struct_st_thumbs_list* prev_thumb_ptr = NULL;

        this_win->wi_thumb->thumbs_open_new_win = true;

/* Needed ?*/
        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;
/**/
        this_win->wi_thumb->thumbs_nb = this_win->wi_data->img.img_total;

        u_int16_t index = 0;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
            char progess_bar_indication[96], thumb_txt[38] = {'\0'};
            bool thumb_valid = false;
            u_int16_t lwidth, lheight;
            u_int8_t* temp_buffer;
            MFDB* temp_mfdb;
            Layer* layer;
            int16_t bar_pos = mul_100_fast(i) / this_win->wi_thumb->thumbs_nb;

            sprintf(progess_bar_indication, "Thumbnail %d/%d", i, this_win->wi_thumb->thumbs_nb);
            st_Progress_Bar_Signal(this_win->wi_progress_bar, bar_pos, (int8_t*)progess_bar_indication);
            if(layerMaskSection) {
                layer = &layerMaskSection->layers[i];
                ExtractLayer(document, &file, &allocator, layer);
                bool layer_visible = layer->isVisible;
                if(layer_visible){                
                    const int left = layer->left > 0 ? layer->left : 0;
                    const int top = layer->top > 0 ? layer->top : 0;
                    const int right = layer->right < document->width ? layer->right : document->width;
                    const int bottom = layer->bottom < document->height ? layer->bottom : document->height;    
                    lwidth = right - left;
                    lheight = bottom - top;
                    if(lwidth > 0 && lheight > 0){
                        temp_buffer = st_ScreenBuffer_Alloc_bpp(lwidth, lheight, 32);
                        temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, lwidth, lheight, 32);
                        st_MFDB_Fill_bpp(temp_mfdb, 0x00FFFFFF, 32);
                        if(!st_Extract_PSD_Layer(document, layer, &allocator, temp_buffer)){
                            mfdb_free(temp_mfdb);
                        }else{
                            thumb_valid = true;
                        }
                    }
                }
            }

            if(!thumb_valid){
                continue;
            }

            if(thumb_ptr == NULL){
                thumb_ptr = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
            } 
            thumb_ptr->thumb_id = i;
            thumb_ptr->thumb_index = i + 1;
            thumb_ptr->thumb_selectable = TRUE;
            thumb_ptr->thumb_visible = TRUE;
            thumb_ptr->next = NULL;
            thumb_ptr->prev = prev_thumb_ptr;
            if(thumb_ptr->prev != NULL){
                thumb_ptr->prev->next = thumb_ptr;
            }

            u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(wanted_width, wanted_height, 32);
            MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, wanted_width, wanted_height, 32);

            st_Rescale_ARGB(temp_mfdb, thumb_original_mfdb, wanted_width, wanted_height);

            // if(!thumb_ptr->thumb_selectable){
                // char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
                // strcpy(font_path, current_path);
                // strcat(font_path, TTF_DEFAULT_PATH);                
            //     sprintf(thumb_txt,"Not Available" );
            //     print_ft_simple(((thumb_original_mfdb->fd_w) >> 1 ) - 40, ((thumb_original_mfdb->fd_h) >> 1 ), thumb_original_mfdb, font_path, 14, thumb_txt);
            // }
            
            // if(strlen((const char*)&layer->name) > 1){
            // char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
            // strcpy(font_path, current_path);
            // strcat(font_path, TTF_DEFAULT_PATH);                
            //     sprintf(thumb_txt,"%s", &layer->name );
            //     print_ft_simple(2, thumb_original_mfdb->fd_h - 2, thumb_original_mfdb, font_path, 14, thumb_txt);
            // }

            if(screen_workstation_bits_per_pixel != 32){
                thumb_ptr->thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                thumb_ptr->thumb_mfdb = thumb_original_mfdb;
            }
            
            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + wanted_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += wanted_height + this_win->wi_thumb->pady;
            thumb_ptr->thumb_selected = FALSE;

            // printf("\n###\tthumb_ptr->thumb_id\t%d\n",thumb_ptr->thumb_id);
            // printf("###\tthumb_ptr->thumb_index\t%d\n",thumb_ptr->thumb_index);
            // if(i != 0){
            // printf("\n###\tthumb_ptr->prev->thumb_id\t%d\n",thumb_ptr->prev->thumb_id);
            // printf("###\tthumb_ptr->prev->thumb_index\t%d\n",thumb_ptr->prev->thumb_index);
            // }

            // printf("###\tthumb_ptr->thumb_id\t%d\n",thumb_ptr->thumb_id);
            // printf("###\tthumb_ptr->thumb_index\t%d\n",thumb_ptr->thumb_index);
            // printf("###\tthumb_ptr->thumb_selectable\t%d\n",thumb_ptr->thumb_selectable);
            // printf("###\tthumb_ptr->thumb_visible\t%d\n",thumb_ptr->thumb_visible);
            // printf("###\tthumb_ptr->thumb_mfdb->fd_w\t%d\n",thumb_ptr->thumb_mfdb->fd_w);
            // printf("###\tthumb_ptr->thumb_mfdb->fd_h\t%d\n",thumb_ptr->thumb_mfdb->fd_h);

            prev_thumb_ptr = thumb_ptr;
            thumb_ptr = NULL;    
            mfdb_free(temp_mfdb);
        }
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        
    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
    }
    DestroyLayerMaskSection(layerMaskSection, &allocator);
    DestroyDocument(document, &allocator);
    file.Close();
}

void _st_Handle_Thumbs_PSD_Generic(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }
    u_int16_t idx = 0;

    MallocAllocator allocator;
    NativeFile file(&allocator);
    if (!file.OpenRead(st_Char_to_WChar(this_win->wi_data->path))) {
        sprintf(alert_message, "Can't load this PSD file");
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
    Document* document = CreateDocument(&file, &allocator);
    if (!document) {
        sprintf(alert_message, "Can't load this PSD file");
        st_form_alert(FORM_STOP, alert_message);        
        file.Close();
        return;
    }

    LayerMaskSection* layerMaskSection = ParseLayerMaskSection(document, &file, &allocator);
    bool hasTransparencyMask = layerMaskSection->hasTransparencyMask;

    this_win->wi_data->img.img_total = layerMaskSection->layerCount;
    this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;

    if(document->colorMode == colorMode::RGB /*&& document->colorMode != colorMode::GRAYSCALE*/ && layerMaskSection->layerCount > 1){
        if(st_form_alert_choice(FORM_QUESTION, (char*)"Do you want to try to extract layers", (char*)"No", (char*)"Yes") == 1){
            this_win->wi_data->thumbnail_slave = false;
            DestroyLayerMaskSection(layerMaskSection, &allocator);
            DestroyDocument(document, &allocator);
            file.Close();                 
            goto end;
        }
    }else{
        goto end;
    }
    if(this_win->wi_data->img.img_total > 1){
        st_Thumb_List_Generic(this_win, "PSD Building layers index", "Layer", 80, 20, 4, 4, TRUE);
    }
end:
    DestroyLayerMaskSection(layerMaskSection, &allocator);
    DestroyDocument(document, &allocator);
    file.Close();    
}