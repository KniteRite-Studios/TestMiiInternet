#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    FILE *fp;
    size_t total_bytes;
    time_t start_time;
    int timeout_seconds;
} download_data_t;

size_t write_callback_with_timeout(void *contents, size_t size, size_t nmemb, void *userp) {
    download_data_t *data = (download_data_t*)userp;
    size_t realsize = size * nmemb;
    
    // Check if 10 seconds have passed
    time_t current_time = time(NULL);
    if (current_time - data->start_time >= data->timeout_seconds) {
        return 0; // Stop download
    }
    
    // Write to file
    size_t written = fwrite(contents, 1, realsize, data->fp);
    data->total_bytes += written;
    
    return written;
}

size_t download_with_timeout(const char *url, const char *filepath, int timeout_seconds) {
    CURL *curl;
    CURLcode res = CURLE_OK; // Initialize to prevent warning
    download_data_t data = {0};
    
    // Open file for writing
    data.fp = fopen(filepath, "wb");
    if (!data.fp) {
        printf("Failed to open file for writing: %s\n", filepath);
        return 0;
    }
    
    data.start_time = time(NULL);
    data.timeout_seconds = timeout_seconds;
    data.total_bytes = 0;
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_with_timeout);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds + 5); // Safety margin
        
        // Add these SSL bypass options:
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    fclose(data.fp);
    
    if (res != CURLE_OK && res != CURLE_WRITE_ERROR) {
        printf("cURL error: %s\n", curl_easy_strerror(res));
    }
    
    return data.total_bytes;
}