#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>     // For net_init(), net_gethostip(), etc.
#include <string.h>      // For strcpy()

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
u8 HWButton = SYS_RETURNTOMENU; // Initialize to a safe default

// Button Definitions for termination. 
void WiiResetPressed(u32 irq, void* ctx) { HWButton = SYS_RETURNTOMENU; }
void WiiPowerPressed() { HWButton = SYS_POWEROFF_STANDBY;}
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
    struct in_addr hostip; // Structure to hold the IP address
    char ip_str[16];       // Buffer to hold the IP address as a string (e.g., "192.168.1.100\0")
    s32 net_result = net_init();
    // --- APP INFO ---
    printf("TestMIiInternet. Internet Speed Test for Wii\n");
    printf("Made by KniteRite Studios. 2025\n");
    printf("Press HOME to exit.\n");

    printf("Initializing network...\n"); // Print to console for debugging
    if (net_result == 0) {
        // Network initialized successfully. Get the assigned IP address.
        hostip = *(struct in_addr *) net_gethostip();
        // Convert the binary IP address to a human-readable string.
        strcpy(ip_str, inet_ntoa(hostip));
        printf("Network initialized.\n"); // Debug print
    } else {
        // Network initialization failed.
        strcpy(ip_str, "Failed"); // Indicate failure
        printf("Network initialization failed! Error: %d\n", net_result); // Debug print
    }

    // Print network information
    printf("Network Information:\n");
    printf("Wii IP Address: %s\n", ip_str);

    while(1) {
        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop
        // this is a "one shot" state which will not fire again until the button has been released
        u32 pressed = WPAD_ButtonsDown(0);

        // Wait for the next frame
        VIDEO_WaitVSync();

        //  Terminate the app.
        if (pressed & WPAD_BUTTON_HOME) {
            printf("Exiting...\n");
            VIDEO_WaitVSync();
            SYS_ResetSystem(HWButton, 0, 0);
        }
    }
    return 0; // END OF LINE
}