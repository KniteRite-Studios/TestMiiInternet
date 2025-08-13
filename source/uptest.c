#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "uptest.h"
#include <ogc/lwp_watchdog.h>
#include <string.h>

typedef struct {
    char *buffer;
    size_t buffer_len;
    size_t total_bytes_sent;
    uint32_t start_time;
    uint32_t end_time;
    int timeout_seconds;
} upload_data_t;

typedef struct {
    char *data;
    size_t size;
} response_data_t;

uint32_t up_time = 0;

uint32_t retrieve_up_time() {
    return up_time;
}

size_t read_mime_callback(char *buffer, size_t size, size_t nitems, void *userp) {
    upload_data_t *upload_data = (upload_data_t*)userp;
    size_t read_bytes = size * nitems;
    
    uint32_t current_time = ticks_to_millisecs(gettime());
    uint32_t elapsed = current_time - upload_data->start_time;
    if (elapsed >= upload_data->timeout_seconds * 1000) {
        return 0;
    }
    
    size_t max_bytes = upload_data->total_bytes_sent - upload_data->buffer_len;
    if (max_bytes > read_bytes)
        max_bytes = read_bytes;

    memcpy(buffer, upload_data->buffer + upload_data->total_bytes_sent, max_bytes);
    upload_data->total_bytes_sent += max_bytes;
    
    return max_bytes;
}

// Callback for capturing response data
size_t write_response_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    response_data_t *response = (response_data_t*)userp;
    size_t realsize = size * nmemb;
    
    char *ptr = realloc(response->data, response->size + realsize + 1);
    if (!ptr) {
        return 0;
    }
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    
    return realsize;
}

size_t upload_with_timeout(int timeout_seconds) {
    upload_data_t upload_data = {0};
    
    // Set up upload data structure
    upload_data.buffer_len = 0x100000; // 1MB;
    upload_data.buffer = malloc(upload_data.buffer_len);
    upload_data.start_time = ticks_to_millisecs(gettime());
    upload_data.timeout_seconds = timeout_seconds;
    upload_data.total_bytes_sent = 0;

    // We don't even have to put anything here, we just want to see how long it takes to upload X bytes of data
    for (int i = 0; i < upload_data.buffer_len; i++)
        upload_data.buffer[i] = -i;
    
    // Set up response capture
    response_data_t response = {0};
    
    CURL *curl = curl_easy_init();
    CURLcode res = CURLE_OK;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://file.io/");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, upload_data.buffer_len);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_mime_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    upload_data.end_time = ticks_to_millisecs(gettime());
    up_time = upload_data.end_time - upload_data.start_time;
    
    if (res != CURLE_OK && res != CURLE_WRITE_ERROR && res != CURLE_OPERATION_TIMEDOUT && res != CURLE_RECV_ERROR) {
        printf("Upload error (%d): %s\n", res, curl_easy_strerror(res));
    }
    
    if (response.data) {
        free(response.data);
    }
    
    free(upload_data.buffer);
    
    return upload_data.total_bytes_sent;
}
