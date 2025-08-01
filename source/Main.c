//TestMiiInternet
//KniteRite Studios
//Masaru Mamoru, PR'd by Abdelali221
//2025, July.
//Modifyable with credit and permission.

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>   // For net_init(), net_gethostip(), etc.
#include <ogc/isfs.h>  // For ISFS_Initialize and ISFS file access
#include <stdio.h>     // For printf, fprintf
#include <stdlib.h>    // For exit
#include <string.h>    // For strcpy()
#include <unistd.h>    
#include <wiisocket.h>

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
    printf("Network de-initialized.\n");
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
    printf("TestMIiInternet. Internet Speed Test for Wii\n");
    printf("Made by KniteRite Studios. 2025\n\n");
    printf("Press HOME to exit.\n");
    printf("=========================================\n");
    printf("Initializing network...\n\n"); // Print to console for debugging
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
        printf("Network initialized.\n\n");
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
                       // Placing it here as it was in your original code.

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

        if (c1 > 0xA0) { active_conn = 1; } 
        else if (c2 > 0xA0) { active_conn = 2; } 
        else if (c3 > 0xA0) { active_conn = 3; }
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
    long http_code = 0;
    double ping_time_ms = do_curl_ping(PING_URL, &http_code);

    if (ping_time_ms >= 0) {
        printf("Ping time to %s: %.2f ms\n", PING_URL, ping_time_ms);
    } else {
        printf("Ping test failed.\n");
    }
    

    // Cleanup before exiting the main loop
    curl_global_cleanup_wrapper(); // Clean up libcurl globally
    ISFS_Deinitialize();           // De-initialize ISFS
    deinit_network();              // De-initialize the network

    printf("\nPress HOME button to exit.\n");

    while(1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        VIDEO_WaitVSync();

        if (pressed & WPAD_BUTTON_HOME) {
            printf("Exiting...\n");
            VIDEO_WaitVSync();
            // All necessary cleanups are done before the loop
            WII_ReturnToMenu(); // Return to the Wii System Menu
            exit(0); //END OF LINE
        }
    }
}
