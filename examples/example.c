#include "example.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "../lib/e-Paper/EPD_IT8951.h"
#include "../lib/GUI/GUI_Paint.h"
#include "../lib/GUI/GUI_BMPfile.h"
#include "../lib/Config/Debug.h"

//#include "../lib/GUI/icons.h"
UBYTE *Refresh_Frame_Buf = NULL;

UBYTE *Panel_Frame_Buf = NULL;
UBYTE *Panel_Area_Frame_Buf = NULL;

bool Four_Byte_Align = false;

extern int epd_mode;
extern UWORD VCOM;
extern UBYTE isColor;
/******************************************************************************
function: Change direction of display, Called after Paint_NewImage()
parameter:
    mode: display mode
******************************************************************************/
static void Epd_Mode(int mode)
{
    if (mode == 3)
    {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_NONE);
        isColor = 1;
    }
    else if (mode == 2)
    {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_HORIZONTAL);
    }
    else if (mode == 1)
    {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_HORIZONTAL);
    }
    else
    {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_NONE);
    }
}

/******************************************************************************
function: Display_ColorPalette_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
****************************************************************************** /
UBYTE Display_ColorPalette_Example(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr)
{
    UWORD In_4bp_Refresh_Area_Width;
    if (Four_Byte_Align == true)
    {
        In_4bp_Refresh_Area_Width = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        In_4bp_Refresh_Area_Width = Panel_Width;
    }
    UWORD In_4bp_Refresh_Area_Height = Panel_Height / 16;

    UDOUBLE Imagesize;

    clock_t In_4bp_Refresh_Start, In_4bp_Refresh_Finish;
    double In_4bp_Refresh_Duration;

    Imagesize = ((In_4bp_Refresh_Area_Width * 4 % 8 == 0) ? (In_4bp_Refresh_Area_Width * 4 / 8) : (In_4bp_Refresh_Area_Width * 4 / 8 + 1)) * In_4bp_Refresh_Area_Height;

    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Debug("Start to demostrate 4bpp palette example\r\n");
    In_4bp_Refresh_Start = clock();

    UBYTE SixteenColorPattern[16] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

    for (int i = 0; i < 16; i++)
    {
        memset(Refresh_Frame_Buf, SixteenColorPattern[i], Imagesize);
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, i * In_4bp_Refresh_Area_Height, In_4bp_Refresh_Area_Width, In_4bp_Refresh_Area_Height, false, Init_Target_Memory_Addr, false);
    }

    In_4bp_Refresh_Finish = clock();
    In_4bp_Refresh_Duration = (double)(In_4bp_Refresh_Finish - In_4bp_Refresh_Start) / CLOCKS_PER_SEC;
    Debug("Write and Show 4bp occupy %f second\n", In_4bp_Refresh_Duration);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }
    return 0;
}

/******************************************************************************
function: Display_CharacterPattern_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
****************************************************************************** /
UBYTE Display_CharacterPattern_Example(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel)
{
    UWORD Display_Area_Width;
    if (Four_Byte_Align == true)
    {
        Display_Area_Width = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        Display_Area_Width = Panel_Width;
    }
    UWORD Display_Area_Height = Panel_Height;

    UWORD Display_Area_Sub_Width = Display_Area_Width / 5;
    UWORD Display_Area_Sub_Height = Display_Area_Height / 5;

    UDOUBLE Imagesize;

    Imagesize = ((Display_Area_Width * BitsPerPixel % 8 == 0) ? (Display_Area_Width * BitsPerPixel / 8) : (Display_Area_Width * BitsPerPixel / 8 + 1)) * Display_Area_Height;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for image memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, Display_Area_Width, Display_Area_Height, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    Paint_Clear(WHITE);

    for (int y = 20; y < Display_Area_Height - Display_Area_Sub_Height; y += Display_Area_Sub_Height) // To prevent arrays from going out of bounds
    {
        for (int x = 20; x < Display_Area_Width - Display_Area_Sub_Width; x += Display_Area_Sub_Width) // To prevent arrays from going out of bounds
        {
            // For color definition of all BitsPerPixel, you can refer to GUI_Paint.h
            Paint_DrawPoint(x + Display_Area_Sub_Width * 3 / 8, y + Display_Area_Sub_Height * 3 / 8, 0x10, DOT_PIXEL_7X7, DOT_STYLE_DFT);
            Paint_DrawPoint(x + Display_Area_Sub_Width * 5 / 8, y + Display_Area_Sub_Height * 3 / 8, 0x30, DOT_PIXEL_7X7, DOT_STYLE_DFT);
            Paint_DrawLine(x + Display_Area_Sub_Width * 3 / 8, y + Display_Area_Sub_Height * 5 / 8, x + Display_Area_Sub_Width * 5 / 8, y + Display_Area_Sub_Height * 5 / 8, 0x50, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
            Paint_DrawRectangle(x, y, x + Display_Area_Sub_Width, y + Display_Area_Sub_Height, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
            Paint_DrawCircle(x + Display_Area_Sub_Width / 2, y + Display_Area_Sub_Height / 2, Display_Area_Sub_Height / 2, 0x50, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
            Paint_DrawNum(x + Display_Area_Sub_Width * 3 / 10, y + Display_Area_Sub_Height * 1 / 4, 1234567890, &Font16, 0x20, 0xE0);
            Paint_DrawString_EN(x + Display_Area_Sub_Width * 3 / 10, y + Display_Area_Sub_Height * 3 / 4, "hello world", &Font16, 0x30, 0xD0);
        }
    }

    switch (BitsPerPixel)
    {
    case BitsPerPixel_8:
    {
        EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width, Display_Area_Height, false, Init_Target_Memory_Addr);
        break;
    }
    case BitsPerPixel_4:
    {
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width, Display_Area_Height, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_2:
    {
        EPD_IT8951_2bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width, Display_Area_Height, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_1:
    {
        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width, Display_Area_Height, A2_Mode, Init_Target_Memory_Addr, false);
        break;
    }
    }

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}

/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_BMP_Example(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel)
{
    UWORD WIDTH;

    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/%dx%d_22.bmp", WIDTH, HEIGHT);

    GUI_ReadBmp(Path, 0, 0);

    // you can draw your character and pattern on the image, for color definition of all BitsPerPixel, you can refer to GUI_Paint.h,
    //   Paint_DrawRectangle(50, 50, WIDTH/2, HEIGHT/2, 0x30, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    // Paint_DrawCircle(WIDTH*3/4, HEIGHT/4, 100, 0xF0, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    //   Paint_DrawNum(WIDTH/4, HEIGHT/5, 709, &Font20, 0x30, 0xB0);

    switch (BitsPerPixel)
    {
    case BitsPerPixel_8:
    {
        // Paint_DrawString_EN(10, 10, "8 bits per pixel 16 grayscale", &Font24, 0xF0, 0x00);
        EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, false, Init_Target_Memory_Addr);
        break;
    }
    case BitsPerPixel_4:
    {
        // Paint_DrawString_EN(10, 10, "4 bits per pixel 16 grayscale", &Font24, 0xF0, 0x00);
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_2:
    {
        // Paint_DrawString_EN(10, 10, "2 bits per pixel 4 grayscale", &Font24, 0xC0, 0x00);
        EPD_IT8951_2bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_1:
    {
        //  Paint_DrawString_EN(10, 10, "1 bit per pixel 2 grayscale", &Font24, 0x80, 0x00);
        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, A2_Mode, Init_Target_Memory_Addr, false);
        break;
    }
    }

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    // DEV_Delay_ms(5000);

    return 0;
}


/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************
/
UBYTE Display_BMP_Patch( UDOUBLE Init_Target_Memory_Addr ,int x, int y, int w, int h)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
  //  UDOUBLE Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    UWORD Panel_Width =  1872;
    UWORD Panel_Height = 1404;
    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/%dx%d_2.bmp", 1872, 1404);


    GUI_ReadBmp(Path, 0, 0 );  

   // Paint_NewImage(Refresh_Frame_Buf, w, h, 0, BLACK);

    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, w, h, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}


/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_BMP_Short( UDOUBLE Init_Target_Memory_Addr ,int x, int y, int w, int h)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
  //  UDOUBLE Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    UWORD Panel_Width =  114;//1872;
    UWORD Panel_Height = 138;// 1404;
    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/2.bmp");

    GUI_ReadBmp(Path, 0, 0);   
    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, WIDTH, HEIGHT, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}





/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_BMP_Patch( UDOUBLE Init_Target_Memory_Addr ,int x, int y, int w, int h)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
  //  UDOUBLE Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    UWORD Panel_Width =  1872;
    UWORD Panel_Height = 1404;
    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/%dx%d_24.bmp", 1872, 1404);

    GUI_ReadBmp2(Path, x, y,w,h);   

    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, w, h, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}





/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_Icon( UDOUBLE Init_Target_Memory_Addr ,int x, int y,  int Icon)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
  //  UDOUBLE Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    UWORD Panel_Width =  1872;
    UWORD Panel_Height = 1404;
    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/%dx%d_24.bmp", 1872, 1404);

    Icon=0;

    
    int w = 1;//BigShip.width;
    int h = 2;//BigShip.height;

    Debug("example befor w:%d h:%d\n",w,h);
    int* mysize=GUI_ReadBmp3(Path, x, y,Icon);   

   // GUI_ReadBmp3(Path, x, y,Icon);   
    Debug("Function over\n");
    w=mysize[0];
   // h=mysize[1];
  //  free(mysize);
    Debug("Exsample After w:%d h:%d\n",w,h);
    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, w, h, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }
  

    return 0;
}




/******************************************************************************
function: Display_BMP_Example
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
****************************************************************************** /
UBYTE Display_Text_Short( UDOUBLE Init_Target_Memory_Addr ,  char *buffer,int x, int y, int w, int h)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
  //  UDOUBLE Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);
    UWORD Panel_Width =  20;//1872;
    UWORD Panel_Height = 20;// 1404;
    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    
    
    //Paint_DrawString_EN(x, y, buffer, &Font24, 0xF0, 0x00);
    
    Paint_DrawString_EN(x, y, "x", &Font24, 0xF0, 0x00);

    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, WIDTH, HEIGHT, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}


/******************************************************************************
function: Display_BMP_HST
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/

/*
UBYTE Display_BMP_HST1(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel, int x, int y)
{
    UWORD WIDTH;

    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
    }
    else
    {
        WIDTH = Panel_Width;
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

    char Path[30];
    sprintf(Path, "./pic/2.bmp");

    GUI_ReadBmp(Path, 0, 0);

    // you can draw your character and pattern on the image, for color definition of all BitsPerPixel, you can refer to GUI_Paint.h,
    //   Paint_DrawRectangle(50, 50, WIDTH/2, HEIGHT/2, 0x30, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    // Paint_DrawCircle(WIDTH*3/4, HEIGHT/4, 100, 0xF0, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    //   Paint_DrawNum(WIDTH/4, HEIGHT/5, 709, &Font20, 0x30, 0xB0);

    switch (BitsPerPixel)
    {
    case BitsPerPixel_8:
    {
        // Paint_DrawString_EN(10, 10, "8 bits per pixel 16 grayscale", &Font24, 0xF0, 0x00);
        EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, y, x, 200, 200, false, Init_Target_Memory_Addr);
        break;
    }
    case BitsPerPixel_4:
    {
        // Paint_DrawString_EN(10, 10, "4 bits per pixel 16 grayscale", &Font24, 0xF0, 0x00);
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, x, y, 200, 200, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_2:
    {
        // Paint_DrawString_EN(10, 10, "2 bits per pixel 4 grayscale", &Font24, 0xC0, 0x00);
        EPD_IT8951_2bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, false, Init_Target_Memory_Addr, false);
        break;
    }
    case BitsPerPixel_1:
    {
        //  Paint_DrawString_EN(10, 10, "1 bit per pixel 2 grayscale", &Font24, 0x80, 0x00);
        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 0, 0, WIDTH, HEIGHT, A2_Mode, Init_Target_Memory_Addr, false);
        break;
    }
    }

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}
 */
/******************************************************************************
function: Display_BMP_HST
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/

UBYTE Display_BMP_Short2( UDOUBLE Init_Target_Memory_Addr ,int x, int y,int icon)
{
    UWORD WIDTH;

    UBYTE BitsPerPixel = BitsPerPixel_8;
 
    UWORD Panel_Width;
    UWORD Panel_Height;
    char Path[30];

    switch (icon)
    {
        case 1:
    Panel_Width =  159;
    Panel_Height = 101;
    sprintf(Path, "./pic/1.bmp");
    break;

    case 2:
    Panel_Width =  114;
    Panel_Height = 138;
    sprintf(Path, "./pic/2.bmp");

        /* code */
        break;

        case 3:
    Panel_Width =  182;
    Panel_Height = 58;
    sprintf(Path, "./pic/3.bmp");
    break;

    default:
    Panel_Width =  114;
    Panel_Height = 138;
    sprintf(Path, "./pic/2.bmp");
        break;
    }


    if (Four_Byte_Align == true)
    {
        WIDTH = Panel_Width - (Panel_Width % 32);
        Debug("Four Bit align");
    }
    else
    {
        WIDTH = Panel_Width;
        Debug("no Four Bit align");
    }
    UWORD HEIGHT = Panel_Height;

    UDOUBLE Imagesize;

    Imagesize = ((WIDTH * BitsPerPixel % 8 == 0) ? (WIDTH * BitsPerPixel / 8) : (WIDTH * BitsPerPixel / 8 + 1)) * HEIGHT;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, WIDTH, HEIGHT, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    // Paint_Clear(WHITE);

   
    

    GUI_ReadBmp(Path, 0, 0);   
    EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, x, y, WIDTH, HEIGHT, false, Init_Target_Memory_Addr);

    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}


/******************************************************************************
function: Dynamic_Refresh_Example
parameter:
    Dev_Info: Information structure read from IT8951
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
******************************************************************************/
UBYTE Dynamic_Refresh_Example(IT8951_Dev_Info Dev_Info, UDOUBLE Init_Target_Memory_Addr)
{
    UWORD Panel_Width = Dev_Info.Panel_W;
    UWORD Panel_Height = Dev_Info.Panel_H;

    UWORD Dynamic_Area_Width = 96;
    UWORD Dynamic_Area_Height = 48;

    UDOUBLE Imagesize;

    UWORD Start_X = 0, Start_Y = 0;

    UWORD Dynamic_Area_Count = 0;

    UWORD Repeat_Area_Times = 0;

    // malloc enough memory for 1bp picture first
    Imagesize = ((Panel_Width * 1 % 8 == 0) ? (Panel_Width * 1 / 8) : (Panel_Width * 1 / 8 + 1)) * Panel_Height;
    if ((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        Debug("Failed to apply for picture memory...\r\n");
        return -1;
    }

    clock_t Dynamic_Area_Start, Dynamic_Area_Finish;
    double Dynamic_Area_Duration;

    while (1)
    {
        Dynamic_Area_Width = 128;
        Dynamic_Area_Height = 96;

        Start_X = 0;
        Start_Y = 0;

        Dynamic_Area_Count = 0;

        Dynamic_Area_Start = clock();
        Debug("Start to dynamic display...\r\n");

        for (Dynamic_Area_Width = 96, Dynamic_Area_Height = 64; (Dynamic_Area_Width < Panel_Width - 32) && (Dynamic_Area_Height < Panel_Height - 24); Dynamic_Area_Width += 32, Dynamic_Area_Height += 24)
        {

            Imagesize = ((Dynamic_Area_Width % 8 == 0) ? (Dynamic_Area_Width / 8) : (Dynamic_Area_Width / 8 + 1)) * Dynamic_Area_Height;
            Paint_NewImage(Refresh_Frame_Buf, Dynamic_Area_Width, Dynamic_Area_Height, 0, BLACK);
            Paint_SelectImage(Refresh_Frame_Buf);
            Epd_Mode(epd_mode);
            Paint_SetBitsPerPixel(1);

            for (int y = Start_Y; y < Panel_Height - Dynamic_Area_Height; y += Dynamic_Area_Height)
            {
                for (int x = Start_X; x < Panel_Width - Dynamic_Area_Width; x += Dynamic_Area_Width)
                {
                    Paint_Clear(WHITE);

                    // For color definition of all BitsPerPixel, you can refer to GUI_Paint.h
                    Paint_DrawRectangle(0, 0, Dynamic_Area_Width - 1, Dynamic_Area_Height, 0x00, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

                    Paint_DrawCircle(Dynamic_Area_Width * 3 / 4, Dynamic_Area_Height * 3 / 4, 5, 0x00, DOT_PIXEL_1X1, DRAW_FILL_FULL);

                    Paint_DrawNum(Dynamic_Area_Width / 4, Dynamic_Area_Height / 4, ++Dynamic_Area_Count, &Font20, 0x00, 0xF0);

                    if (epd_mode == 2)
                        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, 1280 - Dynamic_Area_Width - x, y, Dynamic_Area_Width, Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                    else if (epd_mode == 1)
                        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, Panel_Width - Dynamic_Area_Width - x - 16, y, Dynamic_Area_Width, Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                    else
                        EPD_IT8951_1bp_Refresh(Refresh_Frame_Buf, x, y, Dynamic_Area_Width, Dynamic_Area_Height, A2_Mode, Init_Target_Memory_Addr, true);
                }
            }
            Start_X += 32;
            Start_Y += 24;
        }

        Dynamic_Area_Finish = clock();
        Dynamic_Area_Duration = (double)(Dynamic_Area_Finish - Dynamic_Area_Start) / CLOCKS_PER_SEC;
        Debug("Write and Show occupy %f second\n", Dynamic_Area_Duration);

        Repeat_Area_Times++;
        if (Repeat_Area_Times > 0)
        {
            break;
        }
    }
    if (Refresh_Frame_Buf != NULL)
    {
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}
