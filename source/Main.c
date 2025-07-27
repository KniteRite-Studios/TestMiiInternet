#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>     // For NET_Init(), net_gethostip(), and will internally include what it needs from lwip/sockets.h
#include <string.h>      // For strcpy()


static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
    //---------------------------------------------------------------------------------
    
        // Initialise the video system
        VIDEO_Init();
    
        // This function initialises the attached controllers
        WPAD_Init();
    
        // --- Add Network Initialization Here ---
        // Ensure struct in_addr is properly defined. network.h should provide it.
        struct in_addr hostip; // Structure to hold the IP address
        char ip_str[16];       // Buffer to hold the IP address as a string (e.g., "192.168.1.100\0")
    
        printf("Initializing network...\n"); // Print to console for debugging
    
        // NET_Init() attempts to connect to the Internet using the Wii's configured settings.
        // It returns 0 on success, or a negative error code on failure.
        s32 net_result = net_init();
    
        if (net_result == 0) {
            // Network initialized successfully. Get the assigned IP address.
            // net_gethostip() returns a pointer to the in_addr structure with the IP.
            hostip = *(struct in_addr *) net_gethostip();
            // Convert the binary IP address to a human-readable string.
            // inet_ntoa is typically found in arpa/inet.h, which network.h might include or replicate.
            // If it complains, you might need to find where DevKitPro puts its definition for inet_ntoa.
            // For now, assume network.h handles it.
            strcpy(ip_str, inet_ntoa(hostip));
            printf("Network initialized. IP: %s\n", ip_str); // Debug print
        } else {
            // Network initialization failed.
            strcpy(ip_str, "Failed"); // Indicate failure
            printf("Network initialization failed! Error: %d\n", net_result); // Debug print
        }
        // --- End Network Initialization ---
    
     // Obtain the preferred video mode from the system
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
    console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    //SYS_STDIO_Report(true);

    // Set up the video registers with the chosen mode
    VIDEO_Configure(rmode);

    // Tell the video hardware where our display memory is
    VIDEO_SetNextFramebuffer(xfb);

    // Clear the framebuffer
    VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

    // Make the display visible
    VIDEO_SetBlack(false);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


    // The console understands VT terminal escape codes
    // This positions the cursor on row 2, column 0
    // we can use variables for this with format codes too
    // e.g. printf ("\x1b[%d;%dH", row, column );

    //KR: Appears to be where the displayed data begins along with button registration and controller data. I was mistaken earlier, the meat is actually the above data. 
    printf("\x1b[2;0H");
    printf("TestMiiInternet. Internet Speed and Ping test for Wii.\n");
    printf("Copyright 2025 KniteRite Studios\n");

    // Display the IP address
    printf("IP Address: %s\n", ip_str); // Display the IP address on the console

    while(1) {

        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop
        // this is a "one shot" state which will not fire again until the button has been released
        u32 pressed = WPAD_ButtonsDown(0);

        // We return to the launcher application via exit
        if ( pressed & WPAD_BUTTON_HOME ) exit(0);

        // Wait for the next frame
        VIDEO_WaitVSync();
    }

    return 0;
}