#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "uptest.h"
#include <ogc/lwp_watchdog.h>
#include <fat.h>
#include <string.h>

typedef struct {
    FILE *file;
    size_t total_bytes_sent;
    size_t file_size;
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
    size_t max_bytes = size * nitems;
    
    uint32_t current_time = ticks_to_millisecs(gettime());
    uint32_t elapsed = current_time - upload_data->start_time;
    if (elapsed >= upload_data->timeout_seconds * 1000) {
        return 0;
    }
    
    size_t bytes_read = fread(buffer, 1, max_bytes, upload_data->file);
    upload_data->total_bytes_sent += bytes_read;
    
    return bytes_read;
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

size_t upload_with_timeout(const char *file_path, int timeout_seconds) {
    // Initialize FAT filesystem for SD card access
    if (!fatInitDefault()) {
        printf("Failed to initialize FAT filesystem\n");
        return 0;
    }
    
    // Try to open the upload test file, create if it doesn't exist
    FILE *test_file = fopen("uptest1mb.dat", "rb");
    if (!test_file) {
        printf("Creating 1MB test file...\n");
        
        // Create the test file
        FILE *create_file = fopen("uptest1mb.dat", "wb");
        if (!create_file) {
            printf("Failed to create test file\n");
            return 0;
        }
        
        // Write 1MB of data
        char buffer[1024];
        // Fill buffer with a simple pattern (added realism.)
        for (int i = 0; i < 1024; i++) {
            buffer[i] = (char)(i % 256);
        }
        
        // Write 1024 chunks of 1KB = 1MB
        for (int i = 0; i < 1024; i++) {
            if (fwrite(buffer, 1, 1024, create_file) != 1024) {
                printf("Error writing to test file\n");
                fclose(create_file);
                return 0;
            }
        }
        
        fclose(create_file);
        printf("Test file created successfully\n");
        
        // Now open it for reading
        test_file = fopen("uptest1mb.dat", "rb");
        if (!test_file) {
            printf("Failed to open created test file\n");
            return 0;
        }
    }
    
    // Get file size
    fseek(test_file, 0, SEEK_END);
    long file_size = ftell(test_file);
    fseek(test_file, 0, SEEK_SET);
    
    
    // Set up upload data structure
    upload_data_t upload_data = {0};
    upload_data.file = test_file;
    upload_data.file_size = file_size;
    upload_data.start_time = ticks_to_millisecs(gettime());
    upload_data.timeout_seconds = timeout_seconds;
    upload_data.total_bytes_sent = 0;
    
    // Set up response capture
    response_data_t response = {0};
    
    CURL *curl = curl_easy_init();
    CURLcode res = CURLE_OK;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://file.io/");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, file_size);
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
    
    if (res != CURLE_OK && res != CURLE_WRITE_ERROR && res != CURLE_OPERATION_TIMEDOUT) {
        printf("Upload error: %s\n", curl_easy_strerror(res));
    }
    
    if (response.data) {
        free(response.data);
    }
    
    fclose(test_file);
    
    return upload_data.total_bytes_sent;
}