#include "cache.h"
#include "utils.h"

#include "../external/md5/md5.h"

#include <errno.h>

void print_hash(uint8_t *p){
    for(unsigned int i = 0; i < 16; ++i){
        printf("%02x", p[i]);
    }
    printf("\n");
}

void rek_mkdir(char *path) {
    char *sep = strrchr(path, '/');
    if(sep != NULL) {
        *sep = 0;
        rek_mkdir(path);
        *sep = '/';
    }
    if(mkdir(path, 0777) && errno != EEXIST)
        fprintf(stderr, "error while trying to create '%s'\n%m\n", path); 
}

FILE *fopen_mkdir(char *path, char *mode) {
    char *sep = strrchr(path, '/');
    if(sep) { 
        char *path0 = strdup(path);
        path0[ sep - path ] = 0;
        rek_mkdir(path0);
        free(path0);
    }
    return fopen(path,mode);
}

bool st_Build_MFDB_Cache(char* filepath, MFDB *data){
    u_int8_t md5hash[16];
    u_int32_t size;
    FILE* dst_file = NULL;
    FILE* src_file = NULL;

    char cachepath[256] = {'\0'};
    sprintf(cachepath, "%s/%d/%s", "cachedir", screen_workstation_bits_per_pixel, basename(filepath));
    // printf("... Building cache in %s\n", cachepath);

    dst_file = fopen(cachepath, "wb+");
    src_file = fopen(filepath, "rb");
    if(dst_file && src_file){
        md5File(src_file, md5hash);
        fclose(src_file);
        if(fwrite(md5hash, sizeof(char), 16, dst_file)){
            size = (MFDB_STRIDE(data->fd_w) * data->fd_h) / (8 / data->fd_nplanes);
            fwrite(data->fd_addr, sizeof(char) , size, dst_file);
            fclose(dst_file);
            return TRUE;
        }
        fclose(dst_file);
        return FALSE;
    }
    fclose(src_file);
    return FALSE;
}

bool st_Load_MFDB_Cache(char* filepath, MFDB* mfdb_dst){

    if(mfdb_dst == NULL){
        printf("Error mfdb_dst == NULL\n");
        return FALSE;
    }

    u_int8_t* dst_data = (u_int8_t*)mfdb_dst->fd_addr;

    if(dst_data == NULL){
        printf("Error dst_data == NULL\n");
        return FALSE;
    }    
    u_int8_t this_md5hash[16] = {'\0'};
    char cachepath[256] = {'\0'};
    sprintf(cachepath, "%s/%d/%s", "cachedir", screen_workstation_bits_per_pixel, basename(filepath));    
    FILE* this_file = fopen(cachepath, "rb");
    if(this_file && !(screen_workstation_bits_per_pixel > 8)){
        if(fread(this_md5hash, 16, 1, this_file)){
            u_int32_t size = (MFDB_STRIDE(mfdb_dst->fd_w) * mfdb_dst->fd_h) / (8 / mfdb_dst->fd_nplanes);
            fread(mfdb_dst->fd_addr, sizeof(char) , size, this_file);          
        }
        // printf("st_Load_MFDB_Cache ");
        // print_hash(this_md5hash);
        fclose(this_file);
        return TRUE;
    }
    fclose(this_file);
    return FALSE;
}

bool st_Check_Cached(char* filepath){
    FILE* src_file = NULL;
    FILE* dst_file = NULL;
    u_int8_t src_md5hash[16] = {'\0'};
    u_int8_t dst_md5hash[16] = {'\0'};
    if(!(screen_workstation_bits_per_pixel > 8)){

        char cachepath[256] = {'\0'};

        src_file = fopen(filepath, "rb");
        md5File(src_file, src_md5hash);
        fclose(src_file);

        sprintf(cachepath, "%s/%d/%s", "cachedir", screen_workstation_bits_per_pixel, basename(filepath));
        // printf("-> Check for cache in %s\n", cachepath);

        dst_file = fopen_mkdir(cachepath, (char*)"rb");
        if(dst_file){
            size_t bytesread = fread(dst_md5hash, sizeof(char), 16, dst_file);
            // printf("Bytes read from %s = %lu\n", cachepath, bytesread);
        } 
        // else {
        //     printf("dst_file is NULL\n");
        // }
        // print_hash(src_md5hash);
        // print_hash(dst_md5hash);
        fclose(dst_file);
        if(!(strncmp((const char*)src_md5hash, (const char*)dst_md5hash, 16))){
            return true;
        } else {
            // printf("strncmp failed\n");
            return false;
        }
    }
    return false;
}