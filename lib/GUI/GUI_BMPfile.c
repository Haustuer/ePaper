/*****************************************************************************
* | File      	:   GUI_BMPfile.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master
*                and enhance portability
*----------------
* |	This version:   V2.0
* | Date        :   2018-11-12
* | Info        :
* 1.Change file name: GUI_BMP.c -> GUI_BMPfile.c
* 2.fix: GUI_ReadBmp()
*   Now Xstart and Xstart can control the position of the picture normally,
*   and support the display of images of any size. If it is larger than
*   the actual display range, it will not be displayed.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include "../Config/Debug.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>//exit()
#include <string.h>//memset()
#include <math.h>//memset()
#include <stdio.h>

//#include "icons.h"

//global variables related to BMP picture display
UBYTE *bmp_dst_buf = NULL;
UBYTE *bmp_src_buf = NULL;
UDOUBLE bmp_width, bmp_height;
UBYTE  bmp_BitCount;
UDOUBLE bytesPerLine;
UDOUBLE imageSize;
UDOUBLE skip;
BMPRGBQUAD  palette[256];
extern UBYTE isColor;

typedef struct {
    int w;
    int h;   
} PicSize;


static void Bitmap_format_Matrix(UBYTE *dst,UBYTE *src)
{
	UDOUBLE i,j,k;
    UBYTE *psrc = src;
    UBYTE *pdst = dst;
    UBYTE *p = psrc;
	UBYTE temp;
	UDOUBLE count;
	
	//Since the bmp storage is from the back to the front, it needs to be converted in reverse order.
	switch(bmp_BitCount)
	{
		case 1:
			pdst += (bmp_width * bmp_height);
			
			for(i=0;i<bmp_height;i++)
			{
				pdst -= bmp_width;
				count = 0;
				for (j=0;j<(bmp_width+7)/8;j++)
				{
					temp = p[j];
					
					for (k=0;k<8;k++)
					{
						pdst[0]= ((temp & (0x80>>k)) >> (7-k));
						count++;
						pdst++;
						if (count == bmp_width)
						{
							break;
						}
					}
				}
				pdst -= bmp_width;
				p += bytesPerLine;
			}
		break;
		case 4:
			pdst += (bmp_width * bmp_height);

			for(i=0;i<bmp_height;i++)
			{
				pdst -= bmp_width;
				count = 0;
				for (j=0;j<(bmp_width+1)/2;j++)
				{
					temp = p[j];
					pdst[0]= ((temp & 0xf0) >> 4);
					count++;
					pdst++;
					if (count == bmp_width)
					{
						break;
					}

					pdst[0] = temp & 0x0f;
					count++;
					pdst++;
					if (count == bmp_width)
					{
						break;
					}
				}
				pdst -= bmp_width;
				p += bytesPerLine;
			}
		break;
		case 8:
			pdst += (bmp_width*bmp_height);
			for(i=0;i<bmp_height;i++)
			{
				p = psrc+(i+1)*bytesPerLine;
				p -= skip;
				for(j=0;j<bmp_width;j++)
				{
					pdst -= 1;
					p -= 1;
					pdst[0] = p[0];
				}
			}
		break;
		case 16:
			pdst += (bmp_width*bmp_height*2);
			for(i=0;i<bmp_height;i++)
			{
				p = psrc+(i+1)*bytesPerLine;
				p -= skip;
				for(j=0;j<bmp_width;j++)
				{
					pdst -= 2;
					p -= 2;
					pdst[0] = p[1];
					pdst[1] = p[0];
				}
			}
		break;
		case 24:
			pdst += (bmp_width*bmp_height*3);
			for(i=0;i<bmp_height;i++)
			{
				p = psrc+(i+1)*bytesPerLine;
				p -= skip;
				for(j=0;j<bmp_width;j++)
				{
					pdst -= 3;
					p -= 3;
					pdst[0] = p[2];
					pdst[1] = p[1];
					pdst[2] = p[0];
				}
			}
		break;
		case 32:
			pdst += (bmp_width*bmp_height*4);
			for(i=0;i<bmp_height;i++)
			{
				p = psrc+(i+1)*bmp_width*4;
				for(j=0;j<bmp_width;j++)
				{
					pdst -= 4;
					p -= 4;
					pdst[0] = p[2];
					pdst[1] = p[1];
					pdst[2] = p[0];
					pdst[3] = p[3];
				}
			}
		break;
		
		default:
		break;
	}	
}


static void DrawMatrix2(UWORD Xstart, UWORD Ystart, UWORD w, UWORD h ,UWORD Width, UWORD High,const UBYTE* Matrix)
{
	UWORD Xpos=0;
	UWORD Ypos=0;
	UWORD i,j,x,y;
	UBYTE R,G,B;
	UBYTE temp1,temp2;
	double Gray;
	//Paint_SetSizeMem(w,h);
	Paint_SetTargetWidth(w);
	UWORD xid,yid;
	for (yid=0,j=Ypos;yid<(High);yid++,j++)
	{
 		y=(Ystart+yid+1)%High;

		
		for (xid=0,i=Xpos;xid<(Width);xid++,i++)
		{
			//x=(xid+Width-w-+1)%Width;	
		//	x=(Width-Xstart-w+xid)%Width;	
			x=(Width-Xstart+xid)%Width;	
			switch(bmp_BitCount)
			{
				case 1:
				case 4:
				case 8:
					R = palette[Matrix[(y*Width+x)]].rgbRed;
					G = palette[Matrix[(y*Width+x)]].rgbGreen;
					B = palette[Matrix[(y*Width+x)]].rgbBlue;
				break;
				
				case 16:
					temp1 = Matrix[(y*Width+x)*2];
					temp2 = Matrix[(y*Width+x)*2+1];
					R = (temp1 & 0x7c)<<1;
					G = (((temp1 & 0x03) << 3 ) | ((temp2&0xe0) >> 5))<<3;
					B = (temp2 & 0x1f)<<3;
				break;
				
				case 24:
					R = Matrix[(y*Width+x)*3];
					G = Matrix[(y*Width+x)*3+1];
					B = Matrix[(y*Width+x)*3+2];
				break;
				
				case 32:
					R = Matrix[(y*Width+x)*4];
					G = Matrix[(y*Width+x)*4+1];
					B = Matrix[(y*Width+x)*4+2];
				break;
				
				default:
				break;
			}
		
			Gray = (R*299 + G*587 + B*114 + 500) / 1000;
            if(isColor && i%3==2)
				Paint_SetPixel(i, j, Gray/2);
			else
				Paint_SetPixel(i, j, Gray);
		}
	}
}




int* DrawMatrix3(UWORD Xstart, UWORD Ystart, UWORD Width, UWORD High,const UBYTE* Matrix)
{
	
	int imWidth=62;
	int imHeight=59;
	
	unsigned int Ship1data[3658] = {
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 9, 10, 7, 5, 14, 14, 14, 16, 16, 16, 16, 16, 16, 
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 14, 14, 16, 16, 16, 16, 16, 16, 
		16, 14, 14, 14, 14, 13, 14, 9, 3, 14, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
		 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 14, 6, 2, 13, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 12, 10, 14, 14, 14, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 13, 8, 2, 10, 14,
		 14, 14, 12, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 13, 3, 14,
		 14, 14, 14, 16, 16, 16, 16, 14, 14, 14, 13, 14, 9, 10, 7, 10, 13, 11, 6, 5, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 9, 4, 9, 14, 14, 14, 14, 16, 16, 16, 13, 14, 14, 14, 14, 8, 14, 10, 5, 1, 8, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 13, 3, 14, 14, 14, 14, 14, 16, 16, 14, 14, 14, 14, 12, 8, 12, 9, 
		 11, 11, 12, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14,
		  14, 4, 8, 14, 14, 14, 14, 16, 16, 14, 13, 14, 7, 5, 3, 12, 10, 5, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 11, 2, 13, 14, 14, 14, 14, 16, 14, 14, 14, 9, 12, 8, 11, 8, 6, 12, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16,
		   16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 13, 3, 6, 14, 13, 14, 14, 14, 14, 14, 14, 13, 
		   14, 14, 14, 10, 14, 9, 13, 13, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		   16, 16, 16, 16, 14, 14, 13, 9, 12, 14, 14, 14, 14, 14, 14, 14, 14, 10, 12, 12, 9, 10, 7, 9, 8, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
		   16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 13, 14, 13, 10, 14, 14, 14, 14, 14, 14, 14, 14, 11, 10, 14, 14, 14, 13, 8, 14, 14, 12, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 11, 5, 14, 5, 9, 7, 12, 14, 14, 14, 14, 14, 14, 13, 6, 6, 4, 4, 3, 1, 9, 12, 13, 12, 11, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 6, 10, 13, 7, 10, 1, 7, 9, 13, 14, 14, 13, 14, 14, 11, 8, 6, 7, 7, 9, 2, 9, 12, 13, 11, 12, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 13, 12, 9, 5, 14, 14, 9, 7, 7, 9, 9, 4, 8, 14, 14, 11, 4, 7, 12, 11, 8, 10, 8, 13, 6, 10, 13, 12, 12, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 8, 4, 4, 0, 7, 11, 13, 4, 4, 13, 6, 14, 13, 6, 4, 8, 10, 12, 14, 14, 11, 6, 10, 6, 14, 12, 4, 7, 14, 14, 14, 13, 10, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 2, 6, 12, 9, 7, 11, 4, 1, 9, 6, 11, 6, 9, 14, 14, 13, 6, 7, 13, 10, 3, 2, 1, 9, 4, 13, 14, 14, 5, 5, 11, 9, 10, 11, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 5, 3, 10, 4, 8, 13, 14, 8, 14, 12, 4, 11, 7, 1, 2, 12, 4, 14, 13, 14, 14, 13, 5, 4, 13, 13, 13, 11, 5, 4, 6, 12, 13, 6, 6, 13, 13, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 10, 12, 9, 11, 10, 4, 6, 14, 14, 10, 11, 13, 5, 11, 6, 6, 13, 9, 3, 1, 9, 14, 14, 12, 5, 8, 14, 14, 14, 14, 14, 14, 14, 11, 7, 8, 12, 9, 5, 13, 14, 14, 12, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 13, 11, 11, 11, 12, 10, 10, 10, 10, 11, 3, 14, 14, 13, 9, 13, 4, 11, 8, 6, 13, 14, 14, 14, 9, 7, 14, 6, 4, 11, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 7, 3, 7, 11, 10, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 14, 14, 14, 14, 14, 13, 13, 11, 7, 14, 14, 12, 12, 6, 9, 9, 10, 13, 14, 14, 14, 13, 14, 14, 13, 9, 12, 11, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 8, 4, 5, 7, 12, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 14, 14, 13, 14, 14, 14, 14, 13, 10, 8, 14, 12, 11, 11, 5, 13, 9, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 11, 7, 9, 11, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 8, 2, 7, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 14, 6, 9, 14, 14, 14, 14, 11, 4, 12, 6, 6, 11, 14, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 3, 2, 7, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 12, 6, 12, 11, 10, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 13, 11, 1, 11, 14, 14, 14, 14, 2, 11, 2, 4, 14, 13, 4, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 12, 8, 10, 12, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 13, 6, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 6, 7, 10, 14, 14, 14, 6, 2, 4, 6, 14, 11, 6, 14, 14, 14, 14, 14, 14, 14, 14, 11, 6, 10, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 8, 14, 14, 14, 12, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 9, 12, 14, 12, 2, 1, 10, 4, 13, 10, 7, 14, 14, 14, 14, 14, 14, 14, 11, 2, 11, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 6, 10, 14, 14, 14, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 12, 11, 9, 3, 2, 0, 4, 2, 11, 9, 6, 12, 14, 14, 14, 14, 14, 14, 2, 10, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 11, 14, 14, 14, 12, 14, 14, 13, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 12, 7, 1, 9, 6, 7, 0, 4, 5, 8, 5, 11, 13, 10, 14, 14, 13, 11, 3, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 10, 13, 14, 14, 14, 11, 11, 13, 8, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 11, 3, 7, 12, 13, 5, 1, 1, 7, 1, 10, 11, 10, 11, 14, 14, 10, 10, 13, 12, 14, 9, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 8, 14, 14, 14, 14, 12, 11, 11, 11, 14, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 9, 0, 13, 14, 11, 9, 0, 1, 0, 8, 12, 9, 7, 13, 14, 8, 9, 13, 11, 14, 11, 9, 14, 9, 12, 14, 14, 14, 14, 14, 14, 6, 14, 14, 14, 14, 11, 7, 12, 14, 14, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 9, 7, 11, 14, 14, 10, 2, 1, 4, 11, 9, 7, 9, 14, 11, 7, 13, 12, 13, 14, 10, 13, 14, 12, 14, 13, 14, 14, 14, 14, 6, 14, 14, 14, 14, 4, 11, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 7, 9, 7, 13, 14, 11, 3, 0, 3, 7, 8, 8, 14, 12, 9, 11, 10, 11, 12, 8, 11, 14, 9, 14, 11, 14, 14, 14, 14, 4, 14, 14, 14, 14, 9, 14, 14, 14, 14, 12, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 6, 9, 14, 14, 12, 14, 6, 1, 4, 9, 10, 12, 13, 9, 2, 6, 3, 11, 11, 13, 12, 6, 13, 11, 13, 14, 13, 14, 5, 12, 14, 14, 9, 13, 14, 14, 14, 13, 6, 12, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 13, 7, 10, 13, 13, 14, 14, 12, 4, 1, 6, 7, 10, 12, 7, 7, 3, 4, 5, 8, 8, 11, 12, 10, 9, 14, 13, 13, 8, 8, 14, 10, 10, 14, 14, 13, 14, 12, 4, 14, 11, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 13, 4, 12, 14, 9, 12, 14, 13, 5, 2, 8, 10, 9, 7, 14, 14, 11, 7, 4, 3, 7, 12, 9, 9, 13, 14, 14, 9, 4, 14, 7, 12, 14, 14, 14, 14, 13, 6, 14, 13, 9, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 13, 5, 9, 4, 7, 9, 14, 5, 9, 3, 8, 8, 9, 13, 14, 14, 14, 14, 9, 4, 2, 7, 8, 9, 9, 13, 10, 6, 5, 4, 10, 12, 11, 10, 14, 14, 7, 13, 5, 4, 11, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 11, 8, 4, 13, 11, 5, 6, 6, 3, 1, 2, 3, 9, 14, 14, 14, 14, 14, 14, 7, 4, 3, 5, 8, 11, 12, 6, 5, 9, 8, 13, 11, 8, 14, 14, 10, 3, 8, 13, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 7, 13, 12, 2, 8, 4, 8, 8, 14, 8, 3, 7, 4, 11, 1, 5, 4, 11, 14, 14, 14, 14, 7, 9, 6, 5, 1, 4, 11, 6, 4, 5, 9, 11, 6, 9, 12, 13, 12, 7, 14, 13, 14, 14, 14, 14, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 13, 7, 4, 6, 12, 2, 5, 9, 11, 5, 1, 10, 8, 7, 3, 4, 10, 13, 13, 14, 6, 9, 6, 8, 9, 2, 0, 5, 8, 3, 6, 6, 2, 2, 6, 10, 10, 8, 7, 14, 14, 14, 11, 10, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 3, 8, 13, 8, 11, 7, 8, 10, 7, 6, 13, 2, 9, 7, 4, 8, 5, 12, 8, 7, 13, 7, 10, 6, 11, 10, 8, 2, 7, 4, 5, 5, 7, 11, 11, 13, 13, 9, 7, 5, 1, 8, 9, 1, 6, 14, 14, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 1, 6, 14, 10, 5, 7, 7, 13, 11, 11, 2, 9, 8, 8, 7, 6, 9, 14, 13, 13, 7, 12, 5, 8, 11, 3, 3, 3, 10, 11, 9, 11, 13, 14, 13, 13, 13, 11, 6, 4, 2, 8, 2, 11, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 9, 2, 6, 14, 3, 6, 10, 7, 5, 11, 4, 7, 7, 9, 6, 10, 4, 14, 14, 14, 5, 12, 6, 11, 9, 2, 8, 8, 9, 13, 14, 13, 10, 10, 10, 10, 14, 11, 12, 12, 4, 12, 4, 9, 14, 14, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 9, 12, 4, 7, 1, 8, 7, 9, 7, 6, 3, 5, 12, 8, 7, 10, 3, 14, 14, 13, 5, 9, 5, 7, 8, 0, 1, 10, 2, 5, 11, 14, 13, 10, 6, 8, 12, 11, 14, 11, 3, 11, 3, 8, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 8, 2, 1, 9, 11, 8, 9, 6, 6, 2, 9, 8, 5, 5, 6, 14, 13, 12, 3, 8, 11, 8, 8, 8, 7, 11, 4, 8, 11, 11, 11, 11, 12, 13, 14, 10, 10, 4, 5, 13, 5, 7, 14, 14, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 11, 7, 0, 7, 10, 6, 12, 12, 8, 1, 6, 6, 6, 3, 6, 14, 14, 12, 4, 12, 13, 6, 11, 12, 8, 8, 8, 7, 14, 13, 11, 8, 12, 9, 9, 2, 10, 7, 7, 13, 12, 3, 14, 14, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 5, 0, 4, 9, 9, 12, 12, 10, 14, 8, 2, 3, 7, 1, 13, 14, 11, 3, 4, 5, 6, 5, 6, 3, 3, 5, 3, 3, 5, 10, 7, 8, 14, 14, 9, 10, 5, 5, 12, 13, 3, 14, 14, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 6, 1, 1, 8, 11, 12, 7, 9, 12, 12, 13, 8, 2, 1, 5, 12, 9, 5, 6, 5, 9, 5, 11, 6, 10, 5, 8, 10, 9, 13, 11, 8, 6, 3, 7, 11, 1, 9, 11, 9, 0, 13, 14, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 3, 3, 5, 8, 13, 13, 11, 8, 12, 12, 12, 10, 11, 8, 7, 6, 2, 4, 2, 2, 4, 11, 6, 6, 3, 4, 5, 9, 4, 8, 11, 11, 13, 14, 12, 1, 7, 14, 12, 2, 11, 13, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 7, 10, 3, 6, 5, 11, 13, 11, 13, 14, 13, 11, 11, 11, 12, 10, 12, 10, 10, 10, 10, 13, 9, 7, 8, 10, 11, 12, 11, 10, 12, 12, 9, 9, 9, 4, 6, 12, 11, 2, 9, 13, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 6, 9, 6, 0, 6, 11, 13, 13, 13, 13, 11, 10, 14, 13, 12, 12, 14, 13, 13, 13, 14, 14, 14, 14, 13, 11, 12, 10, 10, 12, 12, 12, 11, 7, 3, 1, 7, 4, 5, 14, 14, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 10, 11, 4, 2, 9, 10, 10, 12, 13, 14, 14, 12, 11, 9, 10, 11, 8, 7, 9, 9, 13, 12, 14, 12, 11, 13, 9, 8, 9, 9, 8, 10, 13, 4, 6, 4, 0, 9, 14, 12, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 13, 12, 10, 3, 6, 11, 10, 7, 7, 13, 13, 13, 12, 11, 9, 13, 10, 10, 12, 9, 11, 12, 10, 10, 11, 8, 11, 13, 14, 14, 13, 11, 2, 3, 11, 1, 11, 14, 13, 10, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 13, 12, 9, 3, 6, 11, 7, 8, 10, 10, 10, 12, 11, 12, 13, 13, 13, 13, 14, 11, 13, 11, 12, 10, 7, 8, 11, 12, 10, 10, 1, 6, 9, 6, 10, 13, 13, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 7, 2, 0, 0, 3, 14, 10, 9, 10, 13, 11, 8, 12, 10, 9, 10, 9, 13, 8, 12, 12, 12, 11, 7, 9, 12, 9, 12, 6, 6, 14, 6, 7, 8, 12, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0, 2, 7, 13, 2, 5, 9, 8, 9, 7, 9, 11, 13, 13, 14, 12, 11, 13, 13, 14, 14, 14, 14, 13, 14, 14, 12, 6, 7, 13, 14, 10, 9, 8, 14, 11, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 9, 10, 9, 13, 2, 5, 14, 14, 13, 9, 10, 8, 8, 7, 6, 8, 6, 7, 8, 10, 10, 10, 9, 8, 5, 2, 5, 13, 14, 13, 7, 1, 9, 12, 11, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 13, 14, 14, 14, 12, 12, 14, 11, 4, 9, 10, 8, 7, 8, 7, 4, 7, 10, 12, 12, 12, 13, 12, 10, 11, 12, 5, 10, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 13, 12, 13, 13, 12, 11, 14, 14, 13, 14, 14, 14, 14, 14, 13, 13, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 11, 14, 14, 14, 14, 14, 14, 16};
		
	
	UWORD Xpos=0;
	UWORD Ypos=0;
	UWORD i,j,x,y;
	UBYTE R,G,B;
	UBYTE temp1,temp2;
	double Gray;
	//Paint_SetSizeMem(w,h);
	Paint_SetTargetWidth(imWidth);
	UWORD xid,yid;
	for (yid=0,j=Ypos;yid<(High);yid++,j++)
	{
 		y=(Ystart+yid+1)%High;

		
		for (xid=0,i=Xpos;xid<(Width);xid++,i++)
		{
			//x=(xid+Width-w-+1)%Width;	
		//	x=(Width-Xstart-w+xid)%Width;	
			x=(Width-Xstart+xid)%Width;	
			switch(bmp_BitCount)
			{
				case 1:
				case 4:
				case 8:
					R = palette[Matrix[(y*Width+x)]].rgbRed;
					G = palette[Matrix[(y*Width+x)]].rgbGreen;
					B = palette[Matrix[(y*Width+x)]].rgbBlue;
				break;
				
				case 16:
					temp1 = Matrix[(y*Width+x)*2];
					temp2 = Matrix[(y*Width+x)*2+1];
					R = (temp1 & 0x7c)<<1;
					G = (((temp1 & 0x03) << 3 ) | ((temp2&0xe0) >> 5))<<3;
					B = (temp2 & 0x1f)<<3;
				break;
				
				case 24:
					R = Matrix[(y*Width+x)*3];
					G = Matrix[(y*Width+x)*3+1];
					B = Matrix[(y*Width+x)*3+2];
				break;
				
				case 32:
					R = Matrix[(y*Width+x)*4];
					G = Matrix[(y*Width+x)*4+1];
					B = Matrix[(y*Width+x)*4+2];
				break;
				
				default:
				break;
			}
			

			Gray = (R*299 + G*587 + B*114 + 500) / 1000;
				int crazyy=y%imWidth;					
				int crazyx=x%imHeight;			

		/*		if (Ship1data[crazyy*imWidth+crazyx]!=16){
					Gray = 0;
					Debug("index:%d   y:%d x:%d  w:%d  Pixel: %d\n",yid*imWidth+xid,yid,xid,*w, Ship1data[yid*imWidth+xid]);
					
				}*/
			
            if(isColor && i%3==2)
				Paint_SetPixel(i, j, Gray/2);
			else
				Paint_SetPixel(i, j, Gray);
		}
	}
	int mysize={imWidth,imHeight};
	return mysize;
}



static void DrawMatrix(UWORD Xpos, UWORD Ypos,UWORD Width, UWORD High,const UBYTE* Matrix)
{
	UWORD i,j,x,y;
	UBYTE R,G,B;
	UBYTE temp1,temp2;
	double Gray;
	
	for (y=0,j=Ypos;y<High;y++,j++)
	{
 		for (x=0,i=Xpos;x<Width;x++,i++)
		{
			switch(bmp_BitCount)
			{
				case 1:
				case 4:
				case 8:
					R = palette[Matrix[(y*Width+x)]].rgbRed;
					G = palette[Matrix[(y*Width+x)]].rgbGreen;
					B = palette[Matrix[(y*Width+x)]].rgbBlue;
				break;
				
				case 16:
					temp1 = Matrix[(y*Width+x)*2];
					temp2 = Matrix[(y*Width+x)*2+1];
					R = (temp1 & 0x7c)<<1;
					G = (((temp1 & 0x03) << 3 ) | ((temp2&0xe0) >> 5))<<3;
					B = (temp2 & 0x1f)<<3;
				break;
				
				case 24:
					R = Matrix[(y*Width+x)*3];
					G = Matrix[(y*Width+x)*3+1];
					B = Matrix[(y*Width+x)*3+2];
				break;
				
				case 32:
					R = Matrix[(y*Width+x)*4];
					G = Matrix[(y*Width+x)*4+1];
					B = Matrix[(y*Width+x)*4+2];
				break;
				
				default:
				break;
			}
		
			Gray = (R*299 + G*587 + B*114 + 500) / 1000;
            if(isColor && i%3==2)
				Paint_SetPixel(i, j, Gray/2);
			else
				Paint_SetPixel(i, j, Gray);
		}
	}
}

UBYTE GUI_ReadBmp(const char *path, UWORD x, UWORD y)
{
	//bmp file pointer
	FILE *fp;
	BMPFILEHEADER FileHead;
	BMPINFOHEADER InfoHead;
	UDOUBLE total_length;
	UBYTE *buf = NULL;
	UDOUBLE ret = -1;
	
	fp = fopen(path,"rb");
	if (fp == NULL)
	{
		return(-1);
	}
 
	ret = fread(&FileHead, sizeof(BMPFILEHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read header error!\n");
		fclose(fp);
		return(-2);
	}

	//Detect if it is a bmp image, since BMP file type is "BM"(0x4D42)
	if (FileHead.bType != 0x4D42)
	{
		Debug("It's not a BMP file\n");
		fclose(fp);
		return(-3);
	}
	
	Debug("*****************************************\n");
	Debug("BMP_bSize:%d \n", FileHead.bSize);
 	Debug("BMP_bOffset:%d \n", FileHead.bOffset);
	
	ret = fread((char *)&InfoHead, sizeof(BMPINFOHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read infoheader error!\n");
		fclose(fp);
		return(-4);
	}
	
	Debug("BMP_biInfoSize:%d \n", InfoHead.biInfoSize);
 	Debug("BMP_biWidth:%d \n", InfoHead.biWidth);
	Debug("BMP_biHeight:%d \n", InfoHead.biHeight);
	Debug("BMP_biPlanes:%d \n", InfoHead.biPlanes);
	Debug("BMP_biBitCount:%d \n", InfoHead.biBitCount);
	Debug("BMP_biCompression:%d \n", InfoHead.biCompression);
	Debug("BMP_bimpImageSize:%d \n", InfoHead.bimpImageSize);
	Debug("BMP_biXPelsPerMeter:%d \n", InfoHead.biXPelsPerMeter);
	Debug("BMP_biYPelsPerMeter:%d \n", InfoHead.biYPelsPerMeter);
	Debug("BMP_biClrUsed:%d \n", InfoHead.biClrUsed);
	Debug("BMP_biClrImportant:%d \n", InfoHead.biClrImportant);
	
	total_length = FileHead.bSize-FileHead.bOffset;
	bytesPerLine=((InfoHead.biWidth*InfoHead.biBitCount+31)>>5)<<2;
	imageSize=bytesPerLine*InfoHead.biHeight;
	skip=(4-((InfoHead.biWidth*InfoHead.biBitCount)>>3))&3;
	
	Debug("bimpImageSize:%d\n", InfoHead.bimpImageSize);
	Debug("total_length:%d\n", total_length);
	Debug("bytesPerLine = %d\n", bytesPerLine);
	Debug("imageSize = %d\n", imageSize);
	Debug("skip = %d\n", skip);
	Debug("*****************************************\n");
	
    bmp_width = InfoHead.biWidth;
    bmp_height = InfoHead.biHeight;
	bmp_BitCount = InfoHead.biBitCount;
	
	//This is old code, but allocate imageSize byte memory is more reasonable
    bmp_src_buf = (UBYTE*)calloc(1,total_length);
	//bmp_src_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_src_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -1;
    }
	//This is old code, but allocate imageSize byte memory is more reasonable
	bmp_dst_buf = (UBYTE*)calloc(1,total_length);
	//bmp_dst_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_dst_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -2;
    }

	 //Jump to data area
    fseek(fp, FileHead.bOffset, SEEK_SET);
	
	//Bytes per line
    buf = bmp_src_buf;
    while ((ret = fread(buf,1,total_length,fp)) >= 0) 
	{
        if (ret == 0) 
		{
            DEV_Delay_us(100);
            continue;
        }
		buf = ((UBYTE*)buf) + ret;
        total_length = total_length - ret;
        if(total_length == 0)
            break;
    }
	
	//Jump to color pattern board
	switch(bmp_BitCount)
	{	
		case 1:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*2,fp);
			if (ret != 8) 
			{
				Debug("Error: fread != 8\n");
				return -5;
			}

			//this is old code, will likely result in memory leak if use 1bp source bmp image
			 
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 4:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*16,fp);
			if (ret != 64) 
			{
				Debug("Error: fread != 64\n");
				return -5;
			}
			//this is old code, will likely result in memory leak if use 4bp source bmp image
			
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 8:
			fseek(fp, 54, SEEK_SET);

			ret = fread(palette,1,4*256,fp);

			if (ret != 1024) 
			{
				Debug("Error: fread != 1024\n");
				return -5;
			}
		break;
		
		default:
		break;
	}

	Bitmap_format_Matrix(bmp_dst_buf,bmp_src_buf);
	
	DrawMatrix(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	//DrawMatrix2(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);

    free(bmp_src_buf);
    free(bmp_dst_buf);

	bmp_src_buf = NULL;
	bmp_dst_buf = NULL;

	fclose(fp);
	return(0);
}



UBYTE GUI_ReadBmp2(const char *path, UWORD x, UWORD y, UWORD w, UWORD h)
{
	//bmp file pointer
	FILE *fp;
	BMPFILEHEADER FileHead;
	BMPINFOHEADER InfoHead;
	UDOUBLE total_length;
	UBYTE *buf = NULL;
	UDOUBLE ret = -1;
	
	fp = fopen(path,"rb");
	if (fp == NULL)
	{
		return(-1);
	}
 
	ret = fread(&FileHead, sizeof(BMPFILEHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read header error!\n");
		fclose(fp);
		return(-2);
	}

	//Detect if it is a bmp image, since BMP file type is "BM"(0x4D42)
	if (FileHead.bType != 0x4D42)
	{
		Debug("It's not a BMP file\n");
		fclose(fp);
		return(-3);
	}
	
	Debug("*****************************************\n");
	Debug("BMP_bSize:%d \n", FileHead.bSize);
 	Debug("BMP_bOffset:%d \n", FileHead.bOffset);
	
	ret = fread((char *)&InfoHead, sizeof(BMPINFOHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read infoheader error!\n");
		fclose(fp);
		return(-4);
	}
	
	Debug("BMP_biInfoSize:%d \n", InfoHead.biInfoSize);
 	Debug("BMP_biWidth:%d \n", InfoHead.biWidth);
	Debug("BMP_biHeight:%d \n", InfoHead.biHeight);
	Debug("BMP_biPlanes:%d \n", InfoHead.biPlanes);
	Debug("BMP_biBitCount:%d \n", InfoHead.biBitCount);
	Debug("BMP_biCompression:%d \n", InfoHead.biCompression);
	Debug("BMP_bimpImageSize:%d \n", InfoHead.bimpImageSize);
	Debug("BMP_biXPelsPerMeter:%d \n", InfoHead.biXPelsPerMeter);
	Debug("BMP_biYPelsPerMeter:%d \n", InfoHead.biYPelsPerMeter);
	Debug("BMP_biClrUsed:%d \n", InfoHead.biClrUsed);
	Debug("BMP_biClrImportant:%d \n", InfoHead.biClrImportant);
	
	total_length = FileHead.bSize-FileHead.bOffset;
	bytesPerLine=((InfoHead.biWidth*InfoHead.biBitCount+31)>>5)<<2;
	imageSize=bytesPerLine*InfoHead.biHeight;
	skip=(4-((InfoHead.biWidth*InfoHead.biBitCount)>>3))&3;
	
	Debug("bimpImageSize:%d\n", InfoHead.bimpImageSize);
	Debug("total_length:%d\n", total_length);
	Debug("bytesPerLine = %d\n", bytesPerLine);
	Debug("imageSize = %d\n", imageSize);
	Debug("skip = %d\n", skip);
	Debug("*****************************************\n");
	
    bmp_width = InfoHead.biWidth;
    bmp_height = InfoHead.biHeight;
	bmp_BitCount = InfoHead.biBitCount;
	
	//This is old code, but allocate imageSize byte memory is more reasonable
    bmp_src_buf = (UBYTE*)calloc(1,total_length);
	//bmp_src_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_src_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -1;
    }
	//This is old code, but allocate imageSize byte memory is more reasonable
	bmp_dst_buf = (UBYTE*)calloc(1,total_length);
	//bmp_dst_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_dst_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -2;
    }

	 //Jump to data area
    fseek(fp, FileHead.bOffset, SEEK_SET);
	
	//Bytes per line
    buf = bmp_src_buf;
    while ((ret = fread(buf,1,total_length,fp)) >= 0) 
	{
        if (ret == 0) 
		{
            DEV_Delay_us(100);
            continue;
        }
		buf = ((UBYTE*)buf) + ret;
        total_length = total_length - ret;
        if(total_length == 0)
            break;
    }
	
	//Jump to color pattern board
	switch(bmp_BitCount)
	{	
		case 1:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*2,fp);
			if (ret != 8) 
			{
				Debug("Error: fread != 8\n");
				return -5;
			}

			//this is old code, will likely result in memory leak if use 1bp source bmp image
			 
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 4:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*16,fp);
			if (ret != 64) 
			{
				Debug("Error: fread != 64\n");
				return -5;
			}
			//this is old code, will likely result in memory leak if use 4bp source bmp image
			
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 8:
			fseek(fp, 54, SEEK_SET);

			ret = fread(palette,1,4*256,fp);

			if (ret != 1024) 
			{
				Debug("Error: fread != 1024\n");
				return -5;
			}
		break;
		
		default:
		break;
	}

	Bitmap_format_Matrix(bmp_dst_buf,bmp_src_buf);
	
	//DrawMatrix(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	DrawMatrix2(x, y,w,h,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);

    free(bmp_src_buf);
    free(bmp_dst_buf);

	bmp_src_buf = NULL;
	bmp_dst_buf = NULL;

	fclose(fp);
	return(0);
}




UBYTE GUI_ReadBmp3(const char *path, UWORD x, UWORD y, UWORD Icon)
{
	//bmp file pointer
	FILE *fp;
	BMPFILEHEADER FileHead;
	BMPINFOHEADER InfoHead;
	UDOUBLE total_length;
	UBYTE *buf = NULL;
	UDOUBLE ret = -1;
	
	fp = fopen(path,"rb");
	if (fp == NULL)
	{
		return(-1);
	}
 
	ret = fread(&FileHead, sizeof(BMPFILEHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read header error!\n");
		fclose(fp);
		return(-2);
	}

	//Detect if it is a bmp image, since BMP file type is "BM"(0x4D42)
	if (FileHead.bType != 0x4D42)
	{
		Debug("It's not a BMP file\n");
		fclose(fp);
		return(-3);
	}
	
	Debug("*****************************************\n");
	Debug("BMP_bSize:%d \n", FileHead.bSize);
 	Debug("BMP_bOffset:%d \n", FileHead.bOffset);
	
	ret = fread((char *)&InfoHead, sizeof(BMPINFOHEADER),1, fp);
	if (ret != 1)
	{
		Debug("Read infoheader error!\n");
		fclose(fp);
		return(-4);
	}
	
	Debug("BMP_biInfoSize:%d \n", InfoHead.biInfoSize);
 	Debug("BMP_biWidth:%d \n", InfoHead.biWidth);
	Debug("BMP_biHeight:%d \n", InfoHead.biHeight);
	Debug("BMP_biPlanes:%d \n", InfoHead.biPlanes);
	Debug("BMP_biBitCount:%d \n", InfoHead.biBitCount);
	Debug("BMP_biCompression:%d \n", InfoHead.biCompression);
	Debug("BMP_bimpImageSize:%d \n", InfoHead.bimpImageSize);
	Debug("BMP_biXPelsPerMeter:%d \n", InfoHead.biXPelsPerMeter);
	Debug("BMP_biYPelsPerMeter:%d \n", InfoHead.biYPelsPerMeter);
	Debug("BMP_biClrUsed:%d \n", InfoHead.biClrUsed);
	Debug("BMP_biClrImportant:%d \n", InfoHead.biClrImportant);
	
	total_length = FileHead.bSize-FileHead.bOffset;
	bytesPerLine=((InfoHead.biWidth*InfoHead.biBitCount+31)>>5)<<2;
	imageSize=bytesPerLine*InfoHead.biHeight;
	skip=(4-((InfoHead.biWidth*InfoHead.biBitCount)>>3))&3;
	
	Debug("bimpImageSize:%d\n", InfoHead.bimpImageSize);
	Debug("total_length:%d\n", total_length);
	Debug("bytesPerLine = %d\n", bytesPerLine);
	Debug("imageSize = %d\n", imageSize);
	Debug("skip = %d\n", skip);
	Debug("*****************************************\n");
	
    bmp_width = InfoHead.biWidth;
    bmp_height = InfoHead.biHeight;
	bmp_BitCount = InfoHead.biBitCount;
	
	//This is old code, but allocate imageSize byte memory is more reasonable
    bmp_src_buf = (UBYTE*)calloc(1,total_length);
	//bmp_src_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_src_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -1;
    }
	//This is old code, but allocate imageSize byte memory is more reasonable
	bmp_dst_buf = (UBYTE*)calloc(1,total_length);
	//bmp_dst_buf = (UBYTE*)calloc(1,imageSize);
    if(bmp_dst_buf == NULL){
        Debug("Load > malloc bmp out of memory!\n");
        return -2;
    }

	 //Jump to data area
    fseek(fp, FileHead.bOffset, SEEK_SET);
	
	//Bytes per line
    buf = bmp_src_buf;
    while ((ret = fread(buf,1,total_length,fp)) >= 0) 
	{
        if (ret == 0) 
		{
            DEV_Delay_us(100);
            continue;
        }
		buf = ((UBYTE*)buf) + ret;
        total_length = total_length - ret;
        if(total_length == 0)
            break;
    }
	
	//Jump to color pattern board
	switch(bmp_BitCount)
	{	
		case 1:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*2,fp);
			if (ret != 8) 
			{
				Debug("Error: fread != 8\n");
				return -5;
			}

			//this is old code, will likely result in memory leak if use 1bp source bmp image
			 
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 4:
			fseek(fp, 54, SEEK_SET);
			ret = fread(palette,1,4*16,fp);
			if (ret != 64) 
			{
				Debug("Error: fread != 64\n");
				return -5;
			}
			//this is old code, will likely result in memory leak if use 4bp source bmp image
			
			bmp_dst_buf = (UBYTE*)calloc(1,InfoHead.biWidth * InfoHead.biHeight);
			if(bmp_dst_buf == NULL)
			{
				Debug("Load > malloc bmp out of memory!\n");
				return -5;
			}
			
		break;
		
		case 8:
			fseek(fp, 54, SEEK_SET);

			ret = fread(palette,1,4*256,fp);

			if (ret != 1024) 
			{
				Debug("Error: fread != 1024\n");
				return -5;
			}
		break;
		
		default:
		break;
	}

	Bitmap_format_Matrix(bmp_dst_buf,bmp_src_buf);
	int mysize[2];
	int w=10;
	int h=20;
	Debug("befor w:%d h:%d\n",w,h);


	//DrawMatrix(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	mysize=DrawMatrix3(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	Debug("After w:%d h:%d\n",w,h);
	w=mysize[0];
	h=mysize[1];
	Debug("more After w:%d h:%d\n",w,h);
    free(bmp_src_buf);
    free(bmp_dst_buf);

	bmp_src_buf = NULL;
	bmp_dst_buf = NULL;

	fclose(fp);
	
	return(0);
}
