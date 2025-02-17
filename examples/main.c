#include "../lib/Config/DEV_Config.h"
#include "example.h"
#include "../lib/GUI/GUI_BMPfile.h"

#include <math.h>

#include <stdlib.h> //exit()
/*  TEst with new libs*/
#include <signal.h> //signal()
#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
/* */
#include <unistd.h>

UWORD VCOM = 1520;
IT8951_Dev_Info Dev_Info = {0, 0};
UWORD Panel_Width;
UWORD Panel_Height;
UDOUBLE Init_Target_Memory_Addr;
int epd_mode = 1; // 1: no rotate, horizontal mirror, for 10.3inch


#define PI 3.14159265358979323846

void mercator_projection(double lat, double lon, int canvas_width, int canvas_height, double min_lat, double center_meridian, int *x, int *y) {
    // Convert latitude and longitude to radians
    double rad_lat = lat * PI / 180.0;
    double rad_lon = lon * PI / 180.0;
    double rad_center_meridian = center_meridian * PI / 180.0;

    // Calculate the Mercator projection
    double x_pos = canvas_width / 2.0 + (rad_lon - rad_center_meridian) * (canvas_width / (2.0 * PI));
    double y_pos = canvas_height / 2.0 - canvas_height / (2.0 * PI) * log(tan(PI / 4.0 + rad_lat / 2.0));

    // Clip coordinates to fit within the canvas
    *x = (int)fmin(fmax(x_pos, 0), canvas_width - 1);
    *y = (int)fmin(fmax(y_pos, 0), canvas_height - 1);
}




void Handler(int signo)
{
    Debug("\r\nHandler:exit\r\n");
    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Debug("free Refresh_Frame_Buf\r\n");
        Refresh_Frame_Buf = NULL;
    }
    if (Panel_Frame_Buf != NULL)
    {
        free(Panel_Frame_Buf);
        Debug("free Panel_Frame_Buf\r\n");
        Panel_Frame_Buf = NULL;
    }
    if (Panel_Area_Frame_Buf != NULL)
    {
        free(Panel_Area_Frame_Buf);
        Debug("free Panel_Area_Frame_Buf\r\n");
        Panel_Area_Frame_Buf = NULL;
    }
    if (bmp_src_buf != NULL)
    {
        free(bmp_src_buf);
        Debug("free bmp_src_buf\r\n");
        bmp_src_buf = NULL;
    }
    if (bmp_dst_buf != NULL)
    {
        free(bmp_dst_buf);
        Debug("free bmp_dst_buf\r\n");
        bmp_dst_buf = NULL;
    }
    if (Dev_Info.Panel_W != 0)
    {
        Debug("Going to sleep\r\n");
        EPD_IT8951_Sleep();
    }
    DEV_Module_Exit();
    exit(0);
}

int main(int argc, char *argv[])
{
    // Exception handling:ctrl + c
    signal(SIGINT, Handler);

    if (argc < 2)
    {
        Debug("Not 2 Args!\r\n");
    }
    if (argc != 3)
    {
        Debug("nor 3 Args!\r\n");
    }

    int temp;
    int temp1;

    if (argc == 3)
    {

        sscanf(argv[1], "%u", &temp);
        sscanf(argv[2], "%u", &temp1);
    }
    else
    {
        temp = 1;
        temp1 = 0;
    }

    Debug("Arg 1:%u\r\n", temp);
    Debug("Arg 2:%u\r\n", temp1);

    // Init the BCM2835 Device
    if (DEV_Module_Init() != 0)
    {
        return -1;
    }
    Debug("VCOM value:%d\r\n", VCOM);
    Debug("Display mode:%d\r\n", epd_mode);
    Dev_Info = EPD_IT8951_Init(VCOM);

    // get some important info from Dev_Info structure
    Panel_Width = Dev_Info.Panel_W;
    Panel_Height = Dev_Info.Panel_H;
    Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    A2_Mode = 6;
    Debug("A2 Mode:%d\r\n", A2_Mode);
    int x;
    int y;

    switch (temp)
    {
    case 1:
        // Show a bmp file
        // 1bp use A2 mode by default, before used it, refresh the screen with WHITE
        // Display_BMP_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_1);
        // Display_BMP_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_2);
        Debug(" Show Pic");
        Display_BMP_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_4);
        break;
    case 2:
        Debug(" Clear Pic");
        EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);
        /* code */
        break;
    case 3:
        /* code */

        Debug("Start of Socket");

        int socket_desc;
        struct sockaddr_in server;
        char *message, server_reply[2000];

        // Create socket
        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_desc == -1)
            Debug("Could not create socket");

        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_family = AF_INET;
        server.sin_port = htons(10443);

        // Connect to remote server
        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("connect error");
            return 1;
        }

        puts("Connected\n");

        // Send some data
        //   message = "GET / HTTP/1.1\r\n\r\n";// copilot

        message = "GET /sprites HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
        if (send(socket_desc, message, strlen(message), 0) < 0)
        {
            puts("Send failed");
            return 1;
        }
        puts("Data Send\n");

        // Receive a reply from the server
        int received_length = recv(socket_desc, server_reply, 2000, 0);
        if (received_length < 0)
        {
            puts("recv failed");
            return 1;
        }
        server_reply[received_length] = '\0'; // Null-terminate the server reply
        puts("Reply received\n");
        puts(server_reply);

        // Find the start of the JSON payload
        char *json_start = strstr(server_reply, "\r\n\r\n");
        if (json_start == NULL)
        {
            puts("Could not find JSON payload");
            return 1;
        }
        json_start += 4; // Skip the "\r\n\r\n"

        // Manually parse the JSON payload
        // Assuming the JSON format is: [{"x":50,"y":25,"icon":2},{"x":20,"y":70,"icon":1},{"x":20,"y":90,"icon":2}]
        const char *delimiter = ",:{}[]\"";
        char *token = strtok(json_start, delimiter);
      
        int icon;
        while (token != NULL)
        {
            if (strcmp(token, "x") == 0)
            {
                token = strtok(NULL, delimiter);
                x = atoi(token);
                printf("x: %d, ", x);
            }
            else if (strcmp(token, "y") == 0)
            {
                token = strtok(NULL, delimiter);
                y = atoi(token);
                printf("y: %d, ", y);
            }
            else if (strcmp(token, "icon") == 0)
            {
                token = strtok(NULL, delimiter);
                icon = atoi(token);
                printf("icon: %d\n", icon);
                Display_BMP_Short2(Init_Target_Memory_Addr, x, y, icon);
                  DEV_Delay_ms(200);

                //Display_BMP_HST(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_4,x,y);

            }
            token = strtok(NULL, delimiter);
        }
        Debug("end of Socket");

      
        break;
        case 7:
        
        



        
        x=300;;
        y=300;
        Display_BMP_Patch(Init_Target_Memory_Addr, x, y, 20, 30);
       break;

       
       case 8:
        
        x=300;;
        y=300;
        Display_BMP_Short(Init_Target_Memory_Addr, x, y, 3, 4);
       break;

       case 9:

       double lat = 45.0;               // Latitude in degrees
       double lon = -122.0;             // Longitude in degrees
       int canvas_width = 1872;          // Canvas width in pixels
       int canvas_height = 1404;         // Canvas height in pixels
       double min_lat = -85.05112878;   // Minimum latitude for Mercator projection
       double center_meridian = 0.0;    // Center meridian in degrees
    
       int x, y;      
    char buffer[40];

    for (double lat = -90.0; lat <= 90.0; lat += 30.0) {
        for (double lon = -180.0; lon <= 180.0; lon += 30.0) {
            snprintf(buffer, 40, "Canvas coordinates: (%d, %d)\n", lat, lon);

            Display_Text_Short(Init_Target_Memory_Addr,buffer,x,y,0,0);
        }
    
    }
    



   
      break;


    default:
        EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, INIT_Mode);

        break;
    }

    /* //Show A2 mode refresh effect
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     Dynamic_Refresh_Example(Dev_Info,Init_Target_Memory_Addr);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);

     //Show how to display a gif, not works well on 6inch e-Paper HAT, 9.7inch e-Paper HAT, others work well
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     Dynamic_GIF_Example(Panel_Width, Panel_Height, Init_Target_Memory_Addr);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);

     //Show how to test frame rate, test it individually,which is related to refresh area size and refresh mode
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     Check_FrameRate_Example(800, 600, Init_Target_Memory_Addr, BitsPerPixel_1);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, A2_Mode);
     EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, GC16_Mode);*/

    // We recommended refresh the panel to white color before storing in the warehouse.
    //    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, INIT_Mode);

    // EPD_IT8951_Standby();
    //   EPD_IT8951_Sleep();

    // In case RPI is transmitting image in no hold mode, which requires at most 10s
    //    DEV_Delay_ms(5000);

    //    DEV_Module_Exit();
    return 0;
}
