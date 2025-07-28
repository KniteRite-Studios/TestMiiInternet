//TestMiiInternet
//KniteRite Studios
//Masaru Mamoru, PR'd by Abdelali221
//2025, July.
//Modifyable with credit and permission.

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>     // For net_init(), net_gethostip(), etc.
#include <string.h>      // For strcpy()
#include <ogc/isfs.h> // For ISFS_Initialize and ISFS file access

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
u8 HWButton = SYS_RETURNTOMENU; // Initialize to a safe default

// Button Definitions for termination.
void WiiResetPressed(u32 irq, void* ctx) { printf("\nboop! you pressed the reset button!"); }
void WiiPowerPressed() { printf("\nboop! you pressed the power button!");}
void WiimotePowerPressed(s32 chan) { HWButton = SYS_POWEROFF_STANDBY; }

int main(int argc, char **argv) {
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
    char ip_str[16] = {0};// Buffer to hold the IP address as a string (e.g., "192.168.1.100\0")
    char gateway[16] = {0};
    char netmask[16] = {0};
    //MORE NETWORK INFO
    char mac_str[18] = {0}; // Buffer to hold the MAC address as a string (e.g., "00:1A:2B:3C:4D:5E\0")

    // --- APP INFO ---


    printf("TestMIiInternet. Internet Speed Test for Wii\n");
    printf("Made by KniteRite Studios. 2025\n\n");
    printf("Press HOME to exit.\n");
    printf("=========================================\n");
    printf("Initializing network...\n\n"); // Print to console for debugging
    s32 net_result = if_config(ip_str, netmask, gateway, TRUE, 2);

    // Function to get the Wii's MAC address as a string
    u8 mac_address_bytes[6] = {0}; // Declare a u8 array to hold the MAC bytes

    // Get MAC address and store in mac_str
    // The net_get_mac_address function directly populates the u8 array.
    // then format it into the mac_str.
    if (net_get_mac_address(mac_address_bytes) == 0) { // Pass the u8 array here
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac_address_bytes[0], mac_address_bytes[1], mac_address_bytes[2],
                 mac_address_bytes[3], mac_address_bytes[4], mac_address_bytes[5]);
    } else {
        strncpy(mac_str, "Unavailable", sizeof(mac_str));
        mac_str[sizeof(mac_str) - 1] = '\0';
    }

    const char *mac_result = mac_str; // While unused, removing it seems to cause problems. tf2 coconut lmao.

    if (net_result == 0) {
        // Network initialized successfully. Get the assigned IP address.
        // Convert the binary IP address to a human-readable string.
        printf("Network initialized.\n\n"); // Debug print
    } else {
        // Network initialization failed.
        strcpy(ip_str, "Failed"); // Indicate failure
        printf("Network initialization failed! Error: %d\n", net_result); // Debug print
    }

    // Print network information
    printf("Network Information:\n\n");
    printf("Wii IP Address: %s\n", ip_str);
    printf("Gateway: %s\n", gateway);
    printf("Netmask: %s\n", netmask);
    printf("MAC Address: %s\n", mac_str);



    // READ ACTIVE CONNECTION NUMBER FROM config.dat (ISFS)
     // Initialize ISFS for NAND file access
    ISFS_Initialize();
    int active_conn = -1;
    FILE *fcfg = fopen("isfs:/shared2/sys/net/02/config.dat", "rb");
    if (fcfg) {
        unsigned char buf[0x1300] = {0};
        size_t read = fread(buf, 1, sizeof(buf), fcfg);
        fclose(fcfg);
        if (read >= 0x1240 + 1) {
            unsigned char c1 = buf[0x0008];
            unsigned char c2 = buf[0x0924];
            unsigned char c3 = buf[0x1240];
            if (c1 == 0xA6 || c1 == 0xA7) {
                active_conn = 1;
            } else if (c2 == 0xA6 || c2 == 0xA7) {
                active_conn = 2;
            } else if (c3 == 0xA6 || c3 == 0xA7) {
                active_conn = 3;
            }
        }
    }
    if (active_conn > 0) {
        printf("\nTesting Network Connection %d.\n", active_conn);
    } else {
        //Check for CRASH or no usable connections
        unsigned char c1 = 0, c2 = 0, c3 = 0;
        FILE *fcfg2 = fopen("isfs:/shared2/sys/net/02/config.dat", "rb");
        if (fcfg2) {
            unsigned char buf2[0x1300] = {0};
            size_t read2 = fread(buf2, 1, sizeof(buf2), fcfg2);
            fclose(fcfg2);
            if (read2 >= 0x1240 + 1) {
                c1 = buf2[0x0008];
                c2 = buf2[0x0924];
                c3 = buf2[0x1240];
            }
        }
        if (c1 == 0x00 && c2 == 0x00 && c3 == 0x00) {
            printf("\nNo Usable Connections!\n");
        } else {
            printf("\nCRASH!\n");
        }
    }
    printf("=========================================\n");




    while(1) {
        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop
        // this is a "one shot" state which will not fire again until the button has been released
        u32 pressed = WPAD_ButtonsDown(0);

        // Wait for the next frame
        VIDEO_WaitVSync();

        // Terminate the app.
        if (pressed & WPAD_BUTTON_HOME) {
            printf("Exiting...\n");
            VIDEO_WaitVSync();
            SYS_ResetSystem(HWButton, 0, 0);
        }
    }
    return 0; // END OF LINE
}
