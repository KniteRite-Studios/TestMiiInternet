#include <stddef.h>

void curl_global_setup(void);
void curl_global_cleanup_wrapper(void);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
double do_curl_ping(const char *url, long *http_code);
size_t download_with_timeout(const char *url, const char *filepath, int timeout_seconds);