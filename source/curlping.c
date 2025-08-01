#include <curl/curl.h>
#include <stdio.h>
#include <stdbool.h>

static int curl_initialized = 0;



void curl_global_setup(void) {
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_initialized = 1;
    }
}

void curl_global_cleanup_wrapper(void) {
    if (curl_initialized) {
        curl_global_cleanup();
        curl_initialized = 0;
    }
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    (void)contents;
    (void)userp;
    return size * nmemb;
}

double do_curl_ping(const char *url, long *http_code) {
    CURL *curl;
    double ping_time_ms = 0.0;
    CURLcode res;

    curl_global_setup();
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ping_time_ms);
            ping_time_ms *= 1000.0;
        } else {
            fprintf(stderr, "cURL error (%d): %s\n", res, curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    return ping_time_ms;
}

