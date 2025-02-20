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

#include "icons.h"

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
	Debug("Draw X:%dY:%d to X*:%dY*:%d\n",Xstart,Ystart,Xstart+w,Ystart+h);

	//Paint_SetSizeMem(w,h);
	Paint_SetTargetWidth(w);
	UWORD xid,yid;
	for (yid=0,j=Ypos;yid<(High);yid++,j++)
	{
 		y=(Ystart+yid)%High;;//%High;   // tinkerd

		  
		for (xid=0,i=Xpos;xid<(Width);xid++,i++)
		{
			//x=(xid+Width-w-+1)%Width;	
		//	x=(Width-Xstart-w+xid)%Width;	
			x=(Width-Xstart+xid-1)%Width;	  // tinkerd
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


int* DrawMatrix3(UWORD Xstart, UWORD Ystart, UWORD Icon ,UWORD Width, UWORD High,const UBYTE* Matrix)
{
	int iconWidth;
	int iconHeight;
	unsigned int *iconData;

	Debug(" Draw Icon:%d\n",Icon);	
	switch (Icon)
	{
		case 2:	
			/* code */
			iconWidth=Ship2W;
			iconHeight=Ship2H;		
			iconData = (unsigned int *)malloc(Ship2W*Ship2H * sizeof(unsigned int));
			memcpy(iconData, Ship2Data, Ship2W*Ship2H * sizeof(unsigned int));
			break;

	case 1:
		/* code */
		iconWidth=Ship1W;
		iconHeight=Ship1H;		
        iconData = (unsigned int *)malloc(Ship1W*Ship1H * sizeof(unsigned int));
        memcpy(iconData, Ship1Data, Ship1W*Ship1H * sizeof(unsigned int));
        break;


		break;
	case 3:
	default:
	iconWidth=SchildiW;
	iconHeight=SchildiH;
	iconData = (unsigned int *)malloc(SchildiW*SchildiH * sizeof(unsigned int));
        memcpy(iconData, SchildiData, SchildiW*SchildiH  * sizeof(unsigned int));
        break;
	}

	
	UWORD Xpos=0;
	UWORD Ypos=0;
	UWORD i,j,x,y;
	UBYTE R,G,B;
	UBYTE temp1,temp2;
	double Gray;
	//Paint_SetSizeMem(w,h);
	Paint_SetTargetWidth(iconWidth+1);
	UWORD xid,yid;
	for (yid=0,j=Ypos;yid<(High);yid++,j++)
	{
 		y=(Ystart+yid)%High;

		
		for (xid=0,i=Xpos;xid<(Width);xid++,i++)
		{
			//x=(xid+Width-w-+1)%Width;	
		//	x=(Width-Xstart-w+xid)%Width;	
		
			x=(Width-Xstart+xid-1)%Width;	
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
			
				const offset=40;
			Gray = (R*299 + G*587 + B*114 + 500) / 1000;
				int crazyy=yid%iconHeight;					
				int crazyx=offset+xid%iconWidth;		

				//Debug("index:%d   y:%d x:%d  w:%d  \n",crazyy*imWidth2+crazyx,yid,xid,imWidth2 );
				if (iconData[crazyy*iconWidth+crazyx]!=16){
					Gray = iconData[crazyy*iconWidth+crazyx]*16;
				//	
					
				}
			
            if(isColor && i%3==2) 
				Paint_SetPixel(i, j, Gray/2);
			else
				Paint_SetPixel(i, j, Gray);
		}
	}
	
	int* mysize = (int*)malloc(2 * sizeof(int));
	 mysize[0]=iconWidth;
	 mysize[1]=iconHeight;	 
	 
	 free(iconData);
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




int* GUI_ReadBmp3(const char *path, UWORD x, UWORD y, UWORD Icon)
{
	Debug("GUI Draw Icon:%d\n",Icon);
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
	
	int w=10;
	int h=20;
	Debug("befor w:%d h:%d\n",w,h);


	//DrawMatrix(x, y,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	int* mysize=DrawMatrix3(x, y,Icon,InfoHead.biWidth, InfoHead.biHeight, bmp_dst_buf);
	Debug("After w:%d h:%d\n",w,h);
	Debug("pointer After w:%d h:%d\n",mysize[0],mysize[1]);
	/*
	if (mysize == NULL) {
		printf("Memory allocation failed\n");
		exit(1);
	}*/
	w=mysize[0];
	h=mysize[1];
	


    free(bmp_src_buf);
    free(bmp_dst_buf);
	free(mysize);  // hmm
	Debug("more After memory clean w:%d h:%d\n",w,h);
	bmp_src_buf = NULL;
	bmp_dst_buf = NULL;

	fclose(fp);
	Debug("befor maloc\n");
	int* picsize = (int*)malloc(2 * sizeof(int));
	Debug("after maloc");
	picsize[0]=w;
	picsize[1]=h;	 
	Debug("more after maloc\n");

	Debug("more After memory clean w:%d h:%d\n",picsize[0],picsize[1]);
	return picsize;
	//return 0;
}