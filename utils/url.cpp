#include "url.h"
#include "../utils/utils.h"

#ifdef WITH_URL

#ifdef WITH_CURL
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}
#endif

bool download_file(char *this_url, char *tmp_file){
    #ifdef WITH_CURL
    CURL *curl_handle;
    FILE *pagefile;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();
    if(curl_handle == NULL){
        return FALSE;
    }
    /* set URL to get here */
    if(curl_easy_setopt(curl_handle, CURLOPT_URL, this_url) != CURLE_OK){
        return FALSE;
    }
    /* Switch on full protocol/debug output while testing */
    if(curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L) != CURLE_OK){
        return FALSE;
    }
    /* disable progress meter, set to 0L to enable it */
    if(curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L) != CURLE_OK){
        return FALSE;
    }
    /* send all data to this function  */
    if(curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data) != CURLE_OK){
        return FALSE;
    }
    /* open the file */
    pagefile = fopen(tmp_file, "wb");
    if(pagefile) {
        /* write the page body to this file handle */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

        /* get it! */
        curl_easy_perform(curl_handle);

        /* close the header file */
        fclose(pagefile);
    } else {
        return FALSE;
    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    curl_global_cleanup();
#else
    sprintf(alert_message,"Error\nURL download not supported");
    st_form_alert(FORM_STOP, alert_message);
#endif
    return TRUE;
}
#endif