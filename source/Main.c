//TestMiiInternet
//KniteRite Studios
//Masaru Mamoru, PR'd by Abdelali221
//2025, August.

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>   // For net_init(), net_gethostip(), etc.
#include <ogc/isfs.h>  // For ISFS_Initialize and ISFS file access
#include <stdio.h>     // For printf, fprintf
#include <stdlib.h>    // For exit
#include <string.h>    // For strcpy()
#include <unistd.h>    
#include <wiisocket.h>
#include "curlutils.h"
#include "downtest.h"
#include "uptest.h"
#include <fat.h>


// External function declarations for functions defined in curlping.c
extern void curl_global_setup(void);
extern void curl_global_cleanup_wrapper(void);
extern double do_curl_ping(const char *url, long *http_code);

static fstats filest __attribute__((aligned(32))); 

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
u8 HWButton = SYS_RETURNTOMENU; // Initialize to a safe default

// Button Definitions for termination.
void WiiResetPressed(u32 irq, void* ctx) { exit(0); }
void WiiPowerPressed() { exit(0);}
void WiimotePowerPressed(s32 chan) { exit(0); }

void POSCursor(uint8_t X, uint8_t Y) {
    printf("\x1b[%d;%dH", Y, X);
}

void ClearScreen() {
    printf("\x1b[2J");
}

// Function to initialize the Wii's network interface
static void init_network() {
    s32 ret;
    ret = net_init();
    if (ret != 0) {
        exit(1); 
    }
}

// Function to de-initialize the network
static void deinit_network() {
    net_deinit();
}

int main(int argc, char **argv) {
    wiisocket_init();
    //---------------------------------------------------------------------------------
    // Button definitions
    SYS_SetResetCallback(WiiResetPressed);
    SYS_SetPowerCallback(WiiPowerPressed);
    WPAD_SetPowerButtonCallback(WiimotePowerPressed);

    // --- VIDEO/CONSOLE INIT FIRST ---
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth-20, rmode->xfbHeight-20, rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // The console understands VT terminal escape codes
    printf("\x1b[2;0H"); // Move cursor to row 2, column 0

    // --- NETWORK INIT AFTER VIDEO/CONSOLE ---
    char ip_str[16] = {0}; // Buffer to hold the IP address as a string (e.g., "192.168.1.100\0")
    char mac_str[18] = {0}; // Buffer to hold the MAC address as a string (e.g., "00:1A:2B:3C:4D:5E\0")
    
    // Initialize the network connection
    init_network(); // Call the helper function

    // --- APP INFO ---
    printf("TestMiiInternet. Internet Speed Test for Wii\n");
    printf("Made by KniteRite Studios. 2025\n");
    printf("Peer Reviewed by Abdelali221.\n");
    printf("=========================================\n");
    printf("Initializing network...\n"); // Print to console for debugging
    s32 net_result = if_config(ip_str, NULL, NULL, TRUE, 2); // if_config handles net_init internally

    u8 mac_address_bytes[6] = {0}; // Declare a u8 array to hold the MAC bytes

    // Get MAC address and store in mac_str
    if (net_get_mac_address(mac_address_bytes) == 0) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac_address_bytes[0], mac_address_bytes[1], mac_address_bytes[2],
                 mac_address_bytes[3], mac_address_bytes[4], mac_address_bytes[5]);
    } else {
        strncpy(mac_str, "Unavailable", sizeof(mac_str));
        mac_str[sizeof(mac_str) - 1] = '\0';
    }

    if (net_result == 0) {
        printf("Network initialized.\n");
    } else {
        strcpy(ip_str, "Failed");
        printf("Network initialization failed! Error: %d\n", net_result);
    }

    // Print network information
    printf("Network Information:\n\n");
    printf("Wii IP Address: %s\n", ip_str);
    printf("MAC Address: %s\n", mac_str);

    // READ ACTIVE CONNECTION NUMBER FROM config.dat (ISFS)
    // Initialize ISFS for NAND file access
    ISFS_Initialize(); // ISFS_Initialize should be called after network init if it relies on it, or before if independent.

    int active_conn = -1;
    s32 fd = ISFS_Open("/shared2/sys/net/02/config.dat", ISFS_OPEN_READ);
    int stat = ISFS_GetFileStats(fd, &filest);
    if (!(fd >= 0 && stat == ISFS_OK)) {
        printf("ISFS_GetFileStats error: %d\n", stat); // More descriptive error message
    }

    if (fd >= 0) { // Check if fd is valid (not -1)
        u8 c1 __attribute__((aligned(32))) = 0;
        u8 c2 __attribute__((aligned(32))) = 0;
        u8 c3 __attribute__((aligned(32))) = 0;

        ISFS_Seek(fd, 8, 0);
        int ret = ISFS_Read(fd, &c1,1);
        if (ret < 0) { printf("Error reading byte 8: %d\n", ret); ISFS_Close(fd); exit(0); }

        ISFS_Seek(fd, 2340, 0);
        ret = ISFS_Read(fd, &c2, 1);
        if (ret < 0) { printf("Error reading byte 2340: %d\n", ret); ISFS_Close(fd); exit(0); }

        ISFS_Seek(fd, 4672, 0);
        ret = ISFS_Read(fd, &c3, 1);
        if (ret < 0) { printf("Error reading byte 4672: %d\n", ret); ISFS_Close(fd); exit(0); }

        if (c1 >= 0xA0) { active_conn = 1; } 
        else if (c2 >= 0xA0) { active_conn = 2; } 
        else if (c3 >= 0xA0) { active_conn = 3; }
        ISFS_Close(fd); // Close the file descriptor after reading
    } else {
        printf("Failed to open config.dat. FD: %d\n", fd); // More descriptive error
        exit(0);
    } 
    
    // Connection Test
    printf("=========================================\n");
    printf("Testing Connection %d...\n", active_conn);
    const char *PING_URL = "http://google.com/"; // GATEWAY IP
    printf("Pinging: %s\n", PING_URL);
    
    // Call the ping function from curlping.c

    double average_ping_time = 0;
    long http_code = 0;
    double ping_time_ms = do_curl_ping(PING_URL, &http_code);
    int successful_tests = 0;

    for (int i = 0; i < 4; i++) {
        ping_time_ms = do_curl_ping(PING_URL, &http_code);

        if (ping_time_ms < 0) {
            printf("Ping test failed.\n");
        } else {
            average_ping_time += ping_time_ms;
            successful_tests++;
        }
    }

    int successful_ping_tests = successful_tests;
    if (successful_tests > 0) {
        printf("Average ping to %s: %.2f ms\n", PING_URL, average_ping_time / successful_tests);
    } else {
        printf("Ping tests failed.\n");
    }
    
    //Download Test
    printf("\nTesting Download Speed... Please wait up to 60 seconds...\n\n");

    const char *DOWNLOAD_URL = "https://www.dropbox.com/scl/fi/c687z55w1vc0k53bd6ua0/downloadtest.dat?rlkey=d1hggbavph5vckpr9kqtiz4bd&st=6up0nd1c&dl=1";
    double total_download_speed = 0;
    int successful_download_tests = 0;

    for (int i = 0; i < 5; i++) {
        printf("\r"); // Move cursor to beginning of line
        double download_speed = (double)download_with_timeout(DOWNLOAD_URL, 15) / (1024.0 * 1024.0); // MB/s
        uint32_t dw_time = retrieve_dw_time();
        
        if (dw_time > 0 && download_speed > 0) {
            double download_speed_mbps = (download_speed * 8) / (dw_time / 1000);
            total_download_speed += download_speed_mbps;
            successful_download_tests++;
            successful_tests++;
            printf("Download speed: %.2f Mbps", download_speed_mbps);
        } else {
            printf("Download test failed.\n");
        }
        fflush(stdout); // Force immediate output
    }
    
    if (successful_download_tests > 0) {
        printf("\nDone.\n");
    } else {
        printf("\nDownload tests failed.\n");
    }

    printf("\nTesting Upload Speed... Please wait up to 60 seconds...\n");

    double total_speed = 0;
    int successful_upload_tests = 0;
    
    for (int i = 0; i < 5; i++) {
        printf("\r"); // Move cursor to beginning of line
        size_t bytes_sent = upload_with_timeout(15);
        uint32_t up_time_ms = retrieve_up_time();
        
        // Work with whatever is actually uploaded (likely 64KB chunks)
        if (up_time_ms > 0 && bytes_sent > 0) {
            double upload_speed_mbps = ((double)bytes_sent * 8) / (1024.0 * 1024.0) / (up_time_ms / 1000.0);
            total_speed += upload_speed_mbps;
            successful_upload_tests++;
            successful_tests++;
        } else {
            printf("Test %d: Failed\n", i+1);
        }
        fflush(stdout);
    }
    
    if (successful_upload_tests > 0) {
        printf("\nUpload Speed: %.2f Mbps\n", 
               total_speed / successful_upload_tests);
        printf("Done.\n");
    } else {
        printf("\nUpload tests failed\n");
    }


    if (successful_tests == 14) {
        printf("\nPING %.2f ms | DOWN %.2f Mbps | UP %.2f Mbps\n", 
               average_ping_time / successful_ping_tests,
               total_download_speed / successful_download_tests,
               total_speed / successful_upload_tests);
    }

    // Cleanup before exiting the main loop
    curl_global_cleanup_wrapper(); // Clean up libcurl globally
    ISFS_Deinitialize();           // De-initialize ISFS
    deinit_network();              // De-initialize the network

    printf("Press HOME button to exit.\n");

    while(1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        VIDEO_WaitVSync();

        if (pressed & WPAD_BUTTON_HOME) {
            printf("Exiting...\n");
            VIDEO_WaitVSync();
            exit(0); //END OF LINE
        }
    }
}