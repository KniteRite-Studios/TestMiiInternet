#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "downtest.h"
#include <ogc/lwp_watchdog.h>

typedef struct {
    size_t total_bytes;
    uint32_t start_time;
    uint32_t end_time;
    int timeout_seconds;
} download_data_t;

size_t write_callback_with_timeout(void *contents, size_t size, size_t nmemb, void *userp) {
    download_data_t *data = (download_data_t*)userp;
    size_t realsize = size * nmemb;
    
    // Check if 10 seconds have passed
    uint32_t current_time = ticks_to_millisecs(gettime());
    if (current_time - data->start_time >= data->timeout_seconds * 1000) {
        return 0; // Stop download
    }

    data->total_bytes += realsize;

    return realsize;
}

uint32_t dw_time = 0;

uint32_t retrieve_dw_time() {
    return dw_time;
}

size_t download_with_timeout(const char *url, int timeout_seconds) {
    CURL *curl;
    CURLcode res = CURLE_OK; // Initialize to prevent warning
    download_data_t data = {0};
    
    // Open file for writing
    
    
    data.start_time = ticks_to_millisecs(gettime());
    data.timeout_seconds = timeout_seconds;
    data.total_bytes = 0;
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_with_timeout);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds); // Safety margin
        
        // Add these SSL bypass options:
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    if (res != CURLE_OK && res != CURLE_WRITE_ERROR) {
        printf("cURL error: %s\n", curl_easy_strerror(res));
    }

    data.end_time = ticks_to_millisecs(gettime());

    dw_time = data.end_time - data.start_time;
    
    return data.total_bytes;
}